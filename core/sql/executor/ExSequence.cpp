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
* RCS:          $Id: exsequence.cpp,v 1.1 2006/11/01 01:44:14 Exp $
* Description:  Class declarations for ExSequence
* Created:
* Modified:     $Date: 2006/11/01 01:44:14 $
* Language:     C++
* Status:       $State: Exp $
*
*
*
*
******************************************************************************
*/

#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ExSequence.h"
#include "ex_expr.h"
#include "ExpSequenceFunction.h"
#include "ExSimpleSqlBuffer.h"
#include "ExStats.h"
#include "memorymonitor.h"
#include "logmxevent.h"

char *GetHistoryRowOLAP(void *data, Int32 n, 
                        NABoolean leading, Lng32 winSize, Int32 &retcode)
{
  ExSequenceTcb *tcb = (ExSequenceTcb*)data;

  retcode = 0;
  if(!leading)
    retcode = -2;

  // For OLAP functions, when fetching the row from the history
  // buffer, it is an error if: 
  //    - The index is negative (a future row, not supported yet)
  //    - The history buffer is too small.  This can only happen if
  //      the buffer is full and the index is beyond the size of the
  //      buffer.  It is not an error if the index is greater than
  //      the buffer size, but the buffer is not yet full.  Until the
  //      buffer is full, we get the right answer.  For example, if
  //      we were using a history buffer of size 1000 and a moving 
  //      window of size 4000.  If the source data only had 100 row,
  //      then we would get the right answer.  No need to raise an error
  //      in this case.
  //
  if (n < 0) {
// this path in the code is a safe garde and may not be hit
// maybe we can chage to assert or may be we can remove in the future
    retcode = -1;   // ERROR condition
    return NULL;
  } else if (n >= tcb->maxNumberHistoryRows_ && 
             tcb->numberHistoryRows_ == tcb->maxNumberHistoryRows_) {
    retcode = -1;   // ERROR condition
    return NULL;
  } else if (n >= tcb->numberHistoryRows_) {
    if(leading || (n - tcb->numberHistoryRows_) >= winSize)
      retcode = 0;
    return NULL;
  }
    
  if ( n <= tcb->currentHistRowInOLAPBuffer_)  // we're within the current buffer
  {
      return tcb->currentHistRowPtr_ - n * tcb->recLen() ; // offset back from first
  }

  // search the previous buffers
  HashBuffer * tmpBuf = tcb->currentOLAPBuffer_->getPrev();
  if (tmpBuf == NULL)
  {
    tmpBuf = tcb->lastOLAPBuffer_;
  }

  n -= tcb->currentHistRowInOLAPBuffer_; // make n relative to buffer top, not to firstHistoryRow
  while ( n > tcb->maxRowsInOLAPBuffer_ ) { // keep shifting one buffer back at a time
      n -= tcb->maxRowsInOLAPBuffer_ ; 
      tmpBuf = tmpBuf->getPrev();
      if (tmpBuf == NULL)
      {
        tmpBuf = tcb->lastOLAPBuffer_;
      }
  }
  return tmpBuf->getFirstRow() + ( tcb->maxRowsInOLAPBuffer_ - n ) * tcb->recLen();
};

char *GetHistoryRowFollowingOLAP(void *data, Int32 n, 
                                 NABoolean leading, Lng32 winSize, Int32 &retcode)
{
  ExSequenceTcb *tcb = (ExSequenceTcb*)data;

  // flip the sign of n for now. The logic to handle negative offsets should be done 
  // in GetHistoryRowPrecedingOLAP() (aka GetHistoryRowOLAP()).
  if ( n < 0 )
    n = -n;

  retcode = 0;
  if(!leading)
    retcode = -2;

  if (winSize !=0)
  {
    if (n == INT_MAX && tcb->isUnboundedFollowing()) 
    {
      return tcb->lastRow_;
    }
    if (n >= tcb->numFollowingRows()) 
    {
      if(!leading || (n - tcb->numFollowingRows()) >= winSize && n != INT_MAX) 
      {
        retcode = 0;
        return NULL;
      } else
        n = tcb->numFollowingRows();
    }
  }
  else
  {
    if (n > tcb->numFollowingRows())
    {
      retcode = -3;
      return NULL;
    }
  }
  n += tcb->currentRetHistRowInOLAPBuffer_;  // n become absolute positive offset (i.e. not relative)

  if ( n < tcb->maxRowsInOLAPBuffer_ ) // we're within the current buffer
    return tcb->currentRetOLAPBuffer_->getFirstRow() + n * tcb->recLen() ;
  
  // search the following buffers
  HashBuffer * tmpBuf = tcb->currentRetOLAPBuffer_;
  do {
    n -= tcb->maxRowsInOLAPBuffer_ ; // shift one buffer back
    tmpBuf = tmpBuf->getNext();
    if (tmpBuf == NULL)
    {
      tmpBuf = tcb->firstOLAPBuffer_;
    }
  }  while ( n >= tcb->maxRowsInOLAPBuffer_ ) ;
  
  return tmpBuf->getFirstRow() + n * tcb->recLen();
};


char *GetHistoryRow(void *data, Int32 n,
                    NABoolean leading, Lng32 winSize, Int32 &retcode)
{
  ExSequenceTcb *tcb = (ExSequenceTcb*)data;

  retcode = 0;

  if (n < 0) return NULL;
  else if (n >= tcb->numberHistoryRows_) return NULL;


  if ( n <= tcb->currentHistRowInOLAPBuffer_)  // we're within the current buffer
  {
    return tcb->currentHistRowPtr_ - n * tcb->recLen() ; // offset back from first
  }
// this code is used for the legacy sequence functions-- Not sure whether to add a test
// for this as the sequence functions are supposed to be replaced by OLAP functions
// I am hiding this code from code coverage tool for now.
  // search the previous buffers
  HashBuffer * tmpBuf = tcb->currentOLAPBuffer_->getPrev();
  if (tmpBuf == NULL)
  {
    tmpBuf = tcb->lastOLAPBuffer_;
  }

  n -= tcb->currentHistRowInOLAPBuffer_; // make n relative to buffer top, not to firstHistoryRow
  while ( n > tcb->maxRowsInOLAPBuffer_ ) { // keep shifting one buffer back at a time
    n -= tcb->maxRowsInOLAPBuffer_ ; 
    tmpBuf = tmpBuf->getPrev();
    if (tmpBuf == NULL)
    {
      tmpBuf = tcb->lastOLAPBuffer_;
    }
  }
  return tmpBuf->getFirstRow() + ( tcb->maxRowsInOLAPBuffer_ - n ) * tcb->recLen();

};

//
// build - construct the TCB subtree for this TDB.
//
ex_tcb * ExSequenceTdb::build(ex_globals * glob)
 {
  // first build the child
  ex_tcb *  child_tcb;
  ExSequenceTcb *sfTcb;
  
  child_tcb = tdbChild_->build(glob);
  
  sfTcb = new(glob->getSpace()) ExSequenceTcb(*this, *child_tcb, glob);
  
  sfTcb->registerSubtasks();
  sfTcb->registerResizeSubtasks();
  
  return (sfTcb);
}


// ExSequenceTcb constructor
//
// 1. Allocate buffer pool.
// 2. Allocate parent queues and initialize private state.
// 3. Fixup expressions.
//
ExSequenceTcb::ExSequenceTcb (const ExSequenceTdb &  myTdb, 
                              const ex_tcb &    child_tcb,
                              ex_globals * glob) : 
  ex_tcb(myTdb, 1, glob),
  lastRow_(NULL),
  clusterDb_(NULL),
  cluster_(NULL),
  ioEventHandler_(NULL),
  OLAPBuffersFlushed_(FALSE),
  firstOLAPBuffer_(NULL),
  lastOLAPBuffer_(NULL),
  rc_(EXE_OK),
  olapBufferSize_(0),
  maxNumberOfOLAPBuffers_(0),
  numberOfOLAPBuffers_(0),
  minNumberOfOLAPBuffers_(0),
  memoryPressureDetected_(FALSE)
{

  Space * space = (glob ? glob->getSpace() : 0);
  CollHeap * heap = (glob ? glob->getDefaultHeap() : 0);
  heap_ = heap;

  childTcb_ = &child_tcb;

  // Allocate the buffer pool
  pool_ = new(space) sql_buffer_pool(myTdb.numBuffers_,
    myTdb.bufferSize_,
    space);

  allocRowLength_ = ROUND8(myTdb.recLen_);


  // Initialize the machinery for maintaining the row history for
  // computing sequence functions.
  //

  maxNumberHistoryRows_ = myTdb.maxHistoryRows_;
  minFollowing_ = myTdb.minFollowing_;
  unboundedFollowing_ = myTdb.isUnboundedFollowing();
  maxNumberOfOLAPBuffers_ = myTdb.maxNumberOfOLAPBuffers_;//testing
  olapBufferSize_ = myTdb.OLAPBufferSize_ ;
  maxRowsInOLAPBuffer_ = myTdb.maxRowsInOLAPBuffer_;
  minNumberOfOLAPBuffers_ = myTdb.minNumberOfOLAPBuffers_;
  numberOfWinOLAPBuffers_ = myTdb.numberOfWinOLAPBuffers_;
  overflowEnabled_ = ! myTdb.isNoOverflow();

  ex_assert( maxNumberOfOLAPBuffers_ >= minNumberOfOLAPBuffers_ ,
	     "maxNumberOfOLAPBuffers is too small");

  // Initialize history parameters
  // For unbounded following -- also create/initialize clusterDb, cluster
  initializeHistory();

  // get the queue that child use to communicate with me
  qchild_  = child_tcb.getParentQueue(); 

  // Allocate the queue to communicate with parent
 
 allocateParentQueues(qparent_,TRUE);
  // Intialized processedInputs_ to the next request to process
  processedInputs_ = qparent_.down->getTailIndex();


  workAtp_ = allocateAtp(myTdb.criDescUp_, space);

  // Fixup the sequence function expression. This requires the standard
  // expression fixup plus initializing the GetRow method for the sequence
  // clauses.
  //
  if (sequenceExpr())
  {
    ((ExpSequenceExpression*)sequenceExpr())->seqFixup
      ((void*)this, GetHistoryRow, GetHistoryRowOLAP);
    sequenceExpr()->fixup(0, getExpressionMode(), this, space, heap_, FALSE, glob);
  }

  if (returnExpr())
  {
    ((ExpSequenceExpression*)returnExpr())->seqFixup
      ((void*)this, GetHistoryRow, GetHistoryRowFollowingOLAP);
    returnExpr()->fixup(0, getExpressionMode(), this, space, heap_, FALSE, glob);
  }

  if (postPred())
    postPred()->fixup(0, getExpressionMode(), this, space, heap_, FALSE, glob);


  if (cancelExpr())
    cancelExpr()->fixup(0, getExpressionMode(), this, space, heap_, FALSE, glob);

  if (checkPartitionChangeExpr())
  {
    ((ExpSequenceExpression*)checkPartitionChangeExpr())->seqFixup
      ((void*)this, GetHistoryRow, GetHistoryRowOLAP);
    checkPartitionChangeExpr()->fixup(0, getExpressionMode(), this, space, heap_, FALSE, glob);
  }

  workAtp_->getTupp(myTdb.tuppIndex_) = new(space) tupp_descriptor;
}

// Destructor
//
//
ExSequenceTcb::~ExSequenceTcb()
{
  if(qparent_.up) delete qparent_.up;
  qparent_.up = NULL;

  if(qparent_.down) delete qparent_.down;
  qparent_.down = NULL;

  if(pool_) delete pool_;
  pool_ = NULL;

  freeResources();
}
  
void ExSequenceTcb::registerSubtasks()
{
    ExScheduler *sched = getGlobals()->getScheduler();
    ex_tcb :: registerSubtasks();
    // Regsiter the I/O event, if overflow is possible
    if ( isUnboundedFollowing() ) {
      ioEventHandler_ = sched->registerNonQueueSubtask(sWork,this); 
      // work around: The handler was just created, while clusterDb was created
      // earlier (in the TCB ctor), so update clusterDb now with the handler
      ex_assert( clusterDb_ , "Unlimited following and no clusterDb_") ;
      clusterDb_->ioEventHandler_ = ioEventHandler_ ;
    }
    // the parent queues will be resizable, so register a resize subtask.
    registerResizeSubtasks();
};

// Free Resources
//
//
void ExSequenceTcb::freeResources()
{
  if (cluster_) delete cluster_;
  cluster_ = NULL;

  if (clusterDb_) delete clusterDb_;
  clusterDb_ = NULL;

  if (lastRow_&&heap_) heap_->deallocateMemory(lastRow_);
  lastRow_ = NULL;

  while ( firstOLAPBuffer_ ) {
    HashBuffer * temp = firstOLAPBuffer_;
    firstOLAPBuffer_ = firstOLAPBuffer_->getNext();
    NADELETEBASIC( temp ,getHeap());
  }
  firstOLAPBuffer_ = NULL;
  lastOLAPBuffer_ = NULL;

  workAtp_->getTupp(myTdb().tuppIndex_).release();
}

// work - doit...
//
//
short ExSequenceTcb::work()
{
  // If there are no parent requests on the queue, then there cannot
  // be anything to do here.
  //
  if (qparent_.down->isEmpty())
    return WORK_OK;

  ex_queue_entry * pentry_down;
  ExSequencePrivateState * pstate;
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
      pstate = (ExSequencePrivateState*) pentry_down->pstate;
      request = pentry_down->downState.request;

      // If the request has already been cancelled don't pass it to the
      // child. Instead, just mark the request as done. This will trigger
      // a EOD reply when this request gets worked on.
      //
      if (request == ex_queue::GET_NOMORE)
        {
          pstate->step_ = ExSeq_DONE;
        }
      else
        {
          pstate->step_ = ExSeq_WORKING_READ;

          // Pass the request to the child
          //
          ex_queue_entry * centry = qchild_.down->getTailEntry();
          centry->downState.request = ex_queue::GET_ALL;
          centry->downState.requestValue = 11;
          centry->downState.parentIndex = processedInputs_;
          centry->passAtp(pentry_down);
          qchild_.down->insert();
        }
    } // end for processedInputs_

  pentry_down = qparent_.down->getHeadEntry();
  pstate = (ExSequencePrivateState*) pentry_down->pstate;
  request = pentry_down->downState.request;
  workAtp_->copyPartialAtp(pentry_down->getAtp(),0,
			   MINOF(myTdb().tuppIndex_,
				 pentry_down->numTuples())-1);
  // copy temp and input tupps. Last tupp history row

  // Take any child replies and process them. Return the processed
  // rows as long the parent queue has room.
  //
  while (1)
    {
      // If we have satisfied the parent request (or it was cancelled),
      // then stop processing rows, cancel any outstanding child
      // requests, and set this request to the CANCELLED state.
      //
      if ((pstate->step_ == ExSeq_WORKING_READ) ||
          (pstate->step_ == ExSeq_WORKING_RETURN))
        {
          if ((request == ex_queue::GET_NOMORE) ||
              ((request == ex_queue::GET_N) &&
               (pentry_down->downState.requestValue 
                <= (Lng32)pstate->matchCount_)))
            {
              qchild_.down->cancelRequestWithParentIndex
                (qparent_.down->getHeadIndex());
              pstate->step_ = ExSeq_CANCELLED;
            }
        }

      switch (pstate->step_)
        {
          // ExSeq_CANCELLED
          //
          // Transition to this state from ...
          // 1. ExSeq_Error - After the error has been processed.
          // 2. ExSeq_Working - If enough rows have been returned.
          // 3. ExSeq_Working - If the request was cancelled.
          //
          // Remain in this state until ..
          // 1. All rows from the child including EOD are consumed
          //
          // Transition from this state to ...
          // 1. ExSeq_DONE - In all cases.
          //
        case ExSeq_CANCELLED:
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

            // If this is the EOD, transition to the ExSeq_DONE state.
            //
            if (child_status == ex_queue::Q_NO_DATA)
                  pstate->step_ = ExSeq_DONE;

            // Discard the child row.
            qchild_.up->removeHead();	    
            break;
          }

        // ExSeq_ERROR
        //
        // Transition to this state from ...
        // 1. ExSeq_WORKING_READ - a child reply with the type SQLERROR.
        // 2. ExSeq_WORKING_RETURN
        // 3. ExSeq_OVERFLOW_READ
        // 4. ExSeq_OVERFLOW_WRITE
        // Remain in this state until ..
        // 1. The error row has been returned to the parent.
        //
        // Transition from this state to ...
        // 1. ExSeq_CANCELLED - In all cases.
        //
        case ExSeq_ERROR:
          {
            // If there is no room in the parent queue for the reply,
            // try again later.
            //
            if (qparent_.up->isFull())
              return WORK_OK;

            ex_queue_entry *pentry_up = qparent_.up->getTailEntry();

            // Cancel the child request - there must be a child request in
            // progress to get to the ExSeq_ERROR state.
            //
            qchild_.down->cancelRequestWithParentIndex
              (qparent_.down->getHeadIndex());

            // Construct and return the error row.
            //
            if (workAtp_->getDiagsArea()) {
              ComDiagsArea * da = workAtp_->getDiagsArea();
              pentry_up->setDiagsArea(da);
              da->incrRefCount();
              workAtp_->setDiagsArea(0);
            }
            pentry_up->upState.status = ex_queue::Q_SQLERROR;
            pentry_up->upState.parentIndex 
              = pentry_down->downState.parentIndex;
            pentry_up->upState.downIndex = qparent_.down->getHeadIndex();
            pentry_up->upState.setMatchNo(pstate->matchCount_);
            qparent_.up->insert();

            // Transition to the ExSeq_CANCELLED state.
            //
            pstate->step_ = ExSeq_CANCELLED;
            break;
          }

        // ExSeq_WORKING_READ
        //
        // Transition to this state from ...
        // 1. ExSeq_EMPTY - If a request is started.
        // 2. ExSeq_WORKING_RETURN - 
        // 3. ExSeq_OVERFLOW_WRITE - 
        // Remain in this state until ...
        // 1. All child replies including EOD have been processed.
        // 2. A SQLERROR row is received.
        // 3. Enough rows have been returned.
        // 4. The request is cancelled.
        // 5. End of partition is reached
        // Transition from this state to ...
	// 2. ExSeq_ERROR - If an SQLERROR rows is received.
        // 3. ExSeq_CANCELLED - If the request is cancelled.
        // 4. ExSeq_WORKING_RETURN
        // 5. ExSeq_OVERFLOW_WRITE - 
        case ExSeq_WORKING_READ:
          {
            if(!isUnboundedFollowing() && isHistoryFull()) {
              pstate->step_ = ExSeq_WORKING_RETURN;
              break;
            }
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
                if ( checkPartitionChangeExpr() &&
                       currentHistRowPtr_)
                {
                  workAtp_->getTupp
                    (myTdb().tuppIndex_).setDataPointer(currentHistRowPtr_);

                  // Check whether the partition changed
                  ex_expr::exp_return_type retCode = checkPartitionChangeExpr()->eval(workAtp_, centry->getAtp());
                  
                  if (retCode == ex_expr::EXPR_ERROR)
                  {
                    updateDiagsArea(centry);
                    pstate->step_ = ExSeq_ERROR;
                    break;
                  }
                  if ( retCode == ex_expr::EXPR_FALSE)
                  {
                    setPartitionEnd(TRUE);
                    pstate->step_ = ExSeq_END_OF_PARTITION;
                    break;
                  }
                }


                if (isUnboundedFollowing() )
                {
                  if (OLAPBuffersFlushed_)
                  {
                    OLAPBuffersFlushed_ = FALSE;// current row is the first one in first buffer already
                  }
                  else
                  {
                    NABoolean noMemory = 
		      advanceHistoryRow( TRUE /* checkMemoryPressure */);
                    if (noMemory)
                    {
                      pstate->step_ = ExSeq_OVERFLOW_WRITE;
                      cluster_->nextBufferToFlush_ = firstOLAPBuffer_;
		      cluster_->afterLastBufferToFlush_ = NULL;//flush them all

		      // If it is the first overflow, for this partition
		      if ( ! memoryPressureDetected_ ) {
			memoryPressureDetected_ = TRUE;
			
		      } // memory pressure detected

                      break;
                    }
                  }
                }
                else
                {
                  advanceHistoryRow();
                }
 
                workAtp_->getTupp
                  (myTdb().tuppIndex_).setDataPointer(currentHistRowPtr_);

                ex_expr::exp_return_type retCode = ex_expr::EXPR_OK;

                // Apply the read phase sequence function expression to compute 
                // the values of the sequence functions.
                if (sequenceExpr())
                {
                  retCode = sequenceExpr()->eval(workAtp_, centry->getAtp());
                  if (retCode == ex_expr::EXPR_ERROR)
                  {
                    updateDiagsArea(centry);
                    pstate->step_ = ExSeq_ERROR;
                    break;
                  }
                }

                // merge the child's diags area into the work atp
                updateDiagsArea(centry);

                qchild_.up->removeHead();

                break;
              }

              // The EOD from the child. Transition to ExSeq_DONE.
              //
            case ex_queue::Q_NO_DATA:
              {
                setPartitionEnd(TRUE);
                if (isHistoryEmpty())
                {
                  pstate->step_ = ExSeq_DONE;
                  qchild_.up->removeHead();
                }
                else
                {
                  pstate->step_ = ExSeq_END_OF_PARTITION;
                }
              }
              break;

              // An SQLERROR from the child. Transition to ExSeq_ERROR.
              //
            case ex_queue::Q_SQLERROR:
              updateDiagsArea(centry);
              pstate->step_ = ExSeq_ERROR;
              break;
            }
          }
          break;
        // ExSeq_WORKING_RETURN
        //
        // Transition to this state from ...
        // 1. ExSeq_WORKING_READ - 
        // 2. ExSeq_OVERFLOW_READ - 
        // 3. ExSeq_END_OF_PARTITION - 
        // Remain in this state until ...
        // 1. All rows are returned.
        // 2. A SQLERROR row is received.
        // 3. Enough rows have been returned.
        //
        // Transition from this state to ...
        // 1. ExSeq_DONE - If all the child rows including EOD have 
        //    been processed.
        // 2. ExSeq_ERROR - If an SQLERROR rows is received.
        // 3. ExSeq_CANCELLED - If enough rows have been returned.
        // 4. ExSeq_CANCELLED - If the request is cancelled.
        // 5. ExSeq_WORKING_RETURN        
        // 6. ExSeq_DONE   
	// 7. ExSeq_OVERFLOW_READ     
        case ExSeq_WORKING_RETURN:
          {
            // If there is not room in the parent Queue for the reply,
            // try again later.
            //
            if (qparent_.up->isFull())
              return WORK_OK;

            if(isHistoryEmpty()) 
            {
              ex_queue_entry * centry = NULL;
              if(!qchild_.up->isEmpty()) 
              {
                centry = qchild_.up->getHeadEntry();
              }
              if(centry && (centry->upState.status == ex_queue::Q_NO_DATA)) 
              {
                pstate->step_ = ExSeq_DONE;
                qchild_.up->removeHead();
              } 
              else 
              {
                pstate->step_ = ExSeq_WORKING_READ;

                if (getPartitionEnd())
                {
                  initializeHistory();
                }
              }
              break;
            }

            if(!canReturnRows() && 
               !getPartitionEnd() &&
               !isUnboundedFollowing() &&
               !isOverflowStarted()) // redundant? because not unbounded ... 
            {
              pstate->step_ = ExSeq_WORKING_READ;
              break;
            }
            
            ex_queue_entry * pentry_up = qparent_.up->getTailEntry();
                
            pentry_up->copyAtp(pentry_down);
            // Try to allocate a tupp.
            //
            if (pool_->get_free_tuple(pentry_up->getTupp(myTdb().tuppIndex_),
                                      recLen()))
              return WORK_POOL_BLOCKED;

            char *tuppData = pentry_up->getTupp
              (myTdb().tuppIndex_).getDataPointer();

            advanceReturnHistoryRow();

            char *histData = currentRetHistRowPtr_; 
            pentry_up->getTupp
              (myTdb().tuppIndex_).setDataPointer(histData);

            ex_expr::exp_return_type retCode = ex_expr::EXPR_OK;
            // Apply the return phase expression
            if(returnExpr())
            {
              retCode = returnExpr()->eval(pentry_up->getAtp(),workAtp_);
              if (retCode == ex_expr::EXPR_ERROR)
              {
                pstate->step_ = ExSeq_ERROR;
                break;
              }
            }

            retCode = ex_expr::EXPR_OK;
            //Apply post predicate expression  
            if (postPred()) 
            {
              retCode = postPred()->eval(pentry_up->getAtp(),pentry_up->getAtp());
              if (retCode == ex_expr::EXPR_ERROR)
              {
                pstate->step_ = ExSeq_ERROR;
                break;
              }
            }

//pentry_up->getAtp()->display("return eval result", myTdb().getCriDescUp());

            //
            // Case-10-030724-7963: we are done pointing the tupp at the
            // history buffer, so point it back to the SQL buffer.
            //
            pentry_up->getTupp
              (myTdb().tuppIndex_).setDataPointer(tuppData);

            switch(retCode) {
            case ex_expr::EXPR_OK:
            case ex_expr::EXPR_TRUE:
            case ex_expr::EXPR_NULL:

              // Copy the row that was computed in the history buffer,
              // to the space previously allocated in the SQL buffer.

              str_cpy_all(tuppData, histData, recLen());

              // Return the processed row.
              //

              // Finalize the queue entry, then insert it
              //
              pentry_up->upState.status = ex_queue::Q_OK_MMORE;
              pentry_up->upState.parentIndex 
                = pentry_down->downState.parentIndex;
              pentry_up->upState.downIndex =
                qparent_.down->getHeadIndex();
              pstate->matchCount_++;
              pentry_up->upState.setMatchNo(pstate->matchCount_);
              qparent_.up->insert();
              break;

              // If the selection predicate returns FALSE,
              // do not return the child row.
              //
            case ex_expr::EXPR_FALSE:
              break;
                    
              // If the selection predicate returns an ERROR,
              // go to the error processing state.
              //
            case ex_expr::EXPR_ERROR:
              pstate->step_ = ExSeq_ERROR;
              break;
            }

            // MV --
            // Now, if there are no errors so far, evaluate the
            // cancel expression
            if ((pstate->step_ != ExSeq_ERROR) && cancelExpr()) 
              {

                // Temporarily point the tupp to the tail of the
                // history buffer for evaluating the
                // expressions.
                //
                pentry_up->getTupp
                  (myTdb().tuppIndex_).setDataPointer(histData);

                retCode = 
                  cancelExpr()->eval(pentry_up->getAtp(),pentry_up->getAtp());
        
                // We are done pointing the tupp at the history
                // buffer, so point it back to the SQL buffer.
                //
                pentry_up->getTupp
                  (myTdb().tuppIndex_).setDataPointer(tuppData);

                if (retCode == ex_expr::EXPR_TRUE)
                  {
                    qchild_.down->cancelRequestWithParentIndex
                      (qparent_.down->getHeadIndex());
                    pstate->step_ = ExSeq_CANCELLED;
                  }
              }

            updateHistRowsToReturn();
            if ( isOverflowStarted() )
            {
              numberOfRowsReturnedBeforeReadOF_ ++;
              if (numberOfRowsReturnedBeforeReadOF_ == maxNumberOfRowsReturnedBeforeReadOF_)
              {
                firstOLAPBufferFromOF_ = currentRetOLAPBuffer_->getNext();
                if (firstOLAPBufferFromOF_ == NULL)
                {
                  firstOLAPBufferFromOF_ = firstOLAPBuffer_;
                }
                for( Int32 i = 0; i < numberOfWinOLAPBuffers_; i++)
                {
                  firstOLAPBufferFromOF_ = firstOLAPBufferFromOF_->getNext();
                  if (firstOLAPBufferFromOF_ == NULL)
                  {
                    firstOLAPBufferFromOF_ = firstOLAPBuffer_;
                  }
                }
                numberOfOLAPBuffersFromOF_ = numberOfOLAPBuffers_ - numberOfWinOLAPBuffers_;

		cluster_->nextBufferToRead_ = firstOLAPBufferFromOF_;
		HashBuffer * afterLast = firstOLAPBufferFromOF_; 
		// last buffer to read into is the current buffer - maybe ?
		for ( Lng32 bufcount = numberOfOLAPBuffersFromOF_ ;
		      bufcount ;
		      bufcount-- ) {
		  afterLast = afterLast->getNext() ;
		  // Don't cycle back if bufcount == 1 because the logic in
		  // Cluster::read relies on the NULL ptr to stop reading
		  if ( bufcount > 1 && ! afterLast ) 
		    afterLast = firstOLAPBuffer_; // cycle back
		}
		// The last buffer to read to is always the current buffer
		// ex_assert ( afterLast == currentRetOLAPBuffer_->getNext(),
		//	    "Miscalculated the last buffer to read into"); 

		cluster_->afterLastBufferToRead_ = afterLast;

                pstate->step_ = ExSeq_OVERFLOW_READ;
              }
            }
          }
          break;

        // ExSeq_END_OF_PARTITION
        //
        // Transition to this state from ...
        // 1. ExSeq_WORKING_READ - 
        // Transition from this state to ...
        // 1. ExSeq_OVERFLOW_WRITE
        // 2. ExSeq_WORKING_RETURN        

        case ExSeq_END_OF_PARTITION:
        {
          setPartitionEnd(TRUE);
          if (lastRow_  && isUnboundedFollowing())
          {
            ex_assert(currentHistRowPtr_ != NULL, "ExSequenceTcb::work() - currentHistRowPtr_ is a NULL pointer");
            str_cpy_all(lastRow_, currentHistRowPtr_, recLen()); 
          }

          if ( isOverflowStarted() ) // we are overflowing
          {
            cluster_->nextBufferToFlush_ = firstOLAPBuffer_;
	    // do not flush beyond the current buffer
	    cluster_->afterLastBufferToFlush_ = currentOLAPBuffer_->getNext();
            pstate->step_ = ExSeq_OVERFLOW_WRITE;
          }
          else
          {
            pstate->step_ = ExSeq_WORKING_RETURN;
          }
        }
        break;

        // ExSeq_OVERFLOW_WRITE
        //
        // Transition to this state from ...
        // 1. ExSeq_WORKING_READ - 
        // 2. ExSeq_END_OF_PARTITION - 
        // Remain in this state until ...
        // 1. OLAPbuffers are written to oveflow space.
        // 2. An error occurs
        //
        // Transition from this state to ...
        // 1. ExSeq_OVERFLOW_READ
        // 2. ExSeq_ERROR - If an error occurs
        case ExSeq_OVERFLOW_WRITE:
        {
          if (!overflowEnabled_)
          {
           // used for debugging when CmpCommon::getDefault(EXE_BMO_DISABLE_OVERFLOW)is set to off ;
            updateDiagsArea(EXE_OLAP_OVERFLOW_NOT_SUPPORTED);
            pstate->step_ = ExSeq_ERROR;
            break;
          }
          ex_assert(isUnboundedFollowing(),"");

          ComDiagsArea *myDiags = NULL;
          if (!cluster_->flush(myDiags, heap_)) {  // flush the buffers
            // if no errors this code path is not visited
            if (myDiags != NULL) 
            { // some error
              updateDiagsArea(myDiags);
              pstate->step_ = ExSeq_ERROR;
              break;
            }
            // not all the buffers are completely flushed. An I/O is pending
            // maybe we cane remove in the future
            return WORK_OK; 
          }

          // At this point -- all the buffers were completely flushed

          OLAPBuffersFlushed_ = TRUE;

          if (getPartitionEnd())
          {
            firstOLAPBufferFromOF_ = firstOLAPBuffer_;
            numberOfOLAPBuffersFromOF_ = numberOfOLAPBuffers_;

	    cluster_->nextBufferToRead_ = firstOLAPBufferFromOF_;
	    // First time we read and fill all the buffers
	    cluster_->afterLastBufferToRead_ = NULL; 

            pstate->step_ = ExSeq_OVERFLOW_READ;  
          }
          else
          {
            pstate->step_ = ExSeq_WORKING_READ;
          }
        }
        break;
        // ExSeq_OVERFLOW_READ
        //
        // Transition to this state from ...
        // 1. ExSeq_OVERFLOW_WRITE  
        // 2. ExSeq_WORKING_RETURN 
        // Remain in this state until ...
        // 1. OLAPbuffers are read from oveflow space.
        // 2. An error occurs
        //
        // Transition from this state to ...
        // 1. ExSeq_WORKING_RETURN
        // 2. ExSeq_ERROR - If an error occurs
         case ExSeq_OVERFLOW_READ:
        {
          assert(firstOLAPBufferFromOF_ &&
                  isUnboundedFollowing() );

          ComDiagsArea *myDiags = NULL;
          if (!cluster_->read(myDiags, heap_)) {
            if (myDiags != NULL) { // some error
              updateDiagsArea(myDiags);
              pstate->step_ = ExSeq_ERROR;
              break;
            }
            // not all the buffers are completely read. An I/O is pending
            return WORK_OK;
          }

            numberOfRowsReturnedBeforeReadOF_ = 0;
            pstate->step_ = ExSeq_WORKING_RETURN;
        }
        break;

        // ExSeq_DONE
        //
        // Transition to the state from ...
        // 1. ExSeq_WORKING_RETURN - if all child rows have been processed.
        // 2. ExSeq_CANCELLED - if all child rows have been consumed.
        // 3. ExSeq_EMPTY - if the request was DOA.
        //
        // Remain in this state until ...
        // 1. The EOD is returned to the parent.
        //
        // Transition from this state to ...
        // 1. ExSeq_EMPTY - In all cases.
        //
        case ExSeq_DONE:
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
	    workAtp_->releasePartialAtp(0,MINOF(myTdb().tuppIndex_,
						pentry_down->numTuples())-1);
            qparent_.down->removeHead();
            qparent_.up->insert();

            // Re-initialize pstate
            //
            pstate->step_ = ExSeq_EMPTY;
            pstate->matchCount_ = 0;

            // Initialize the history buffer in preparation for the
            // next request.
            //
            initializeHistory();

            // If there are no more requests, simply return.
            //
            if (qparent_.down->isEmpty())
              return WORK_OK;
           
            // If we haven't given to our child the new head
            // index return and ask to be called again.
            //
            if (qparent_.down->getHeadIndex() == processedInputs_)
              return WORK_CALL_AGAIN;

            // Position at the new head of the request queue.
            //
            pentry_down = qparent_.down->getHeadEntry();
            pstate = (ExSequencePrivateState*) pentry_down->pstate;
            request = pentry_down->downState.request; 
          }
        break;
        } // switch pstate->step_
    } // while
}



NABoolean ExSequenceTcb::advanceHistoryRow(NABoolean checkMemoryPressure) 
{
  histRowsToReturn_++;

  if (currentOLAPBuffer_ == NULL)
  {
    currentOLAPBuffer_ = firstOLAPBuffer_;
    ex_assert (currentHistRowPtr_ == NULL,"ExSequenceTcb::advanceHistoryRow() - currentHistRowPtr_ is not a NULL pointer");
    currentHistRowPtr_ = currentOLAPBuffer_->getFirstRow() ;
    currentHistRowInOLAPBuffer_=0;
  }
  else
  {
    currentHistRowInOLAPBuffer_++;

    if ( currentHistRowInOLAPBuffer_ == maxRowsInOLAPBuffer_ ) 
    {  // reached end of this buffer
      currentHistRowInOLAPBuffer_ = 0;

      if ( currentOLAPBuffer_->getNext() == NULL  ) 
      { //last buffer
        if ( canAllocateOLAPBuffer() && 
	     ! isOverflowStarted() && // once overflowing - no more new buffers
	     addNewOLAPBuffer(checkMemoryPressure) )
	  {
	    currentOLAPBuffer_ = lastOLAPBuffer_;
	    currentHistRowPtr_ = currentOLAPBuffer_->getFirstRow();
	  }
        else
	  {
	    currentOLAPBuffer_ = firstOLAPBuffer_;
	    currentHistRowPtr_ = currentOLAPBuffer_->getFirstRow();
	    return TRUE; // no memory -- start overflow
	  }
      }
      else
      {
        currentOLAPBuffer_ = currentOLAPBuffer_->getNext();
        currentHistRowPtr_ = currentOLAPBuffer_->getFirstRow();
      }
    }
    else
    {
      currentHistRowPtr_ = currentHistRowPtr_ + recLen();
    }
  }

  if (numberHistoryRows_ < maxNumberHistoryRows_) 
  {
    numberHistoryRows_++;
  }
  return FALSE;
};

void ExSequenceTcb::advanceReturnHistoryRow() 
{

  if (currentRetOLAPBuffer_ == NULL)  //initially currentRetOLAPBuffer_ is null
  {
    ex_assert(firstOLAPBuffer_ !=NULL,"ExSequenceTcb::advanceReturnHistoryRow() - firstOLAPBuffer is a NULL pointer");
    currentRetOLAPBuffer_ = firstOLAPBuffer_;
    ex_assert(currentRetHistRowPtr_ == NULL,"ExSequenceTcb::advanceReturnHistoryRow() - currentRetHistRowPtr_ is not a NULL pointer");
    currentRetHistRowPtr_ = currentRetOLAPBuffer_->getFirstRow();
    currentRetHistRowInOLAPBuffer_ = 0;
    return;
  }
  currentRetHistRowInOLAPBuffer_ ++;

  if ( currentRetHistRowInOLAPBuffer_ == maxRowsInOLAPBuffer_ ) 
  {  // reached end of this buffer
    currentRetHistRowInOLAPBuffer_ = 0;

    if ( currentRetOLAPBuffer_->getNext() == NULL  ) 
    { //last buffer
      currentRetOLAPBuffer_ = firstOLAPBuffer_;
      currentRetHistRowPtr_ = currentRetOLAPBuffer_->getFirstRow();
    }
    else
    {
      currentRetOLAPBuffer_ = currentRetOLAPBuffer_->getNext();
      currentRetHistRowPtr_ = currentRetOLAPBuffer_->getFirstRow();
    }
  }
  else
  {
    currentRetHistRowPtr_ = currentRetHistRowPtr_ + recLen();
  }
};

NABoolean  ExSequenceTcb::removeOLAPBuffer()
{

  if (lastOLAPBuffer_ == NULL || firstOLAPBuffer_ == NULL)
  {
    return FALSE;
  }

  if (lastOLAPBuffer_ == firstOLAPBuffer_)
  {
   // since we keep a minimum number of buffers this code won't be visited
   // this code should not be removed. if we decide to not keep a min number 
   // of buffers in the list it will be used
    NADELETEBASIC( lastOLAPBuffer_ ,heap_);
    numberOfOLAPBuffers_ = 0;
    maxNumberHistoryRows_ = myTdb().maxHistoryRows_;
    lastOLAPBuffer_ = NULL;
    firstOLAPBuffer_ = NULL;
    return TRUE;
  }

  HashBuffer * tmpBuf = lastOLAPBuffer_;
  lastOLAPBuffer_ = lastOLAPBuffer_->getPrev();
  NADELETEBASIC( tmpBuf ,heap_);
  lastOLAPBuffer_->setNext(NULL);
  numberOfOLAPBuffers_ --;
  maxNumberHistoryRows_ = numberOfOLAPBuffers_ * maxRowsInOLAPBuffer_;
  return TRUE;
}

NABoolean ExSequenceTcb::shrinkOLAPBufferList()
{
  while ( numberOfOLAPBuffers_ > minNumberOfOLAPBuffers_ ) 
  {
    if (! removeOLAPBuffer())
    {
      return FALSE;// error
    }
  };
  return TRUE;
}
// Return FALSE if memory allocation failed.
NABoolean  ExSequenceTcb::addNewOLAPBuffer(NABoolean checkMemoryPressure)
{
  if ( checkMemoryPressure && 
       ! clusterDb_->enoughMemory(olapBufferSize_) ) 
    return FALSE; // not enought memory (e.g., memory pressure)

  HashBuffer *tmpBuf;
  if ( cluster_ ) 
  {
    tmpBuf = new(heap_) HashBuffer(cluster_);
  }
  else 
  {
    tmpBuf = new(heap_) 
	 HashBuffer(olapBufferSize_, allocRowLength_, false, heap_, clusterDb_, &rc_);
  }
  if ( tmpBuf == NULL || 
       tmpBuf->getDataPointer() == NULL ) 
  {
    return FALSE; // no memory
  }

  if (firstOLAPBuffer_== NULL)
  {
    firstOLAPBuffer_ = tmpBuf;
    lastOLAPBuffer_ = firstOLAPBuffer_;
  }
  else
  {
    lastOLAPBuffer_->setNext(tmpBuf);
    tmpBuf->setPrev(lastOLAPBuffer_) ;
    lastOLAPBuffer_ = tmpBuf;
  }
  numberOfOLAPBuffers_++;
  maxNumberHistoryRows_ = numberOfOLAPBuffers_ * maxRowsInOLAPBuffer_;
  if(isUnboundedFollowing())
  {
    maxNumberOfRowsReturnedBeforeReadOF_ = 
        (numberOfOLAPBuffers_ - numberOfWinOLAPBuffers_) * maxRowsInOLAPBuffer_;
  }
  return TRUE; // no memory problems
}

void ExSequenceTcb::initializeHistory()
{
  currentOLAPBuffer_ = NULL;
  currentHistRowInOLAPBuffer_ = 0;
  currentHistRowPtr_ = NULL;

  currentRetOLAPBuffer_ = NULL;
  currentRetHistRowInOLAPBuffer_ = 0;
  currentRetHistRowPtr_ = NULL;

  numberHistoryRows_ = 0;
  histRowsToReturn_ = 0;
  partitionEnd_ = FALSE;

  // allocate the minimum number of olap buffers
  if (firstOLAPBuffer_ == NULL)
  {
    for (Int32 i = 0 ; i < minNumberOfOLAPBuffers_; i++)
    {
      if ( ! addNewOLAPBuffer( FALSE /* No Memory Pressure Check */ ) ) 
        ex_assert(0, "No memory for minimal OLAP window!");
    }
  }  
  else
  {
    if (!shrinkOLAPBufferList())
      ex_assert(0,"initializeHistory-- can not shrink buffer list");
  }

  // Initialize all the settings needed for unbounded following (and overflow)
  if ( isUnboundedFollowing() )
  {
      //last row in partition
    if (lastRow_ == NULL)
    {
      lastRow_ = new(heap_) char[recLen()];
      ex_assert( lastRow_ , "No memory available for OLAP Operator");
    }

    // initialize parameters needed for overflow handling
    OLAPBuffersFlushed_ = FALSE;
    
    firstOLAPBufferFromOF_ = NULL;
    numberOfOLAPBuffersFromOF_ = 0;
    
    memoryPressureDetected_ = FALSE;
    numberOfRowsReturnedBeforeReadOF_ = 0;

    createCluster();

    // Cluster needs to know the first buffer in the OLAP list of buffers
    // (Currently only used when reading buffers from overflow for
    //  "bounded following", where Cluster::read() may need to jump back to
    //  first buffer in the list.)
    cluster_->firstBufferInList_ = firstOLAPBuffer_ ;

  } // if ( isUnboundedFollowing() )
}

void ExSequenceTcb::createCluster()
{
  MemoryMonitor * memMonitor = 
    getGlobals()->castToExExeStmtGlobals()->getMemoryMonitor();
  ULng32 availableMemory = memMonitor->getPhysMemInBytes() / 100
    * myTdb().memUsagePercent_;
  
  // if quota, and it's less than avail memory, then use that lower figure 
  if ( myTdb().memoryQuotaMB() > 0 &&
	 myTdb().memoryQuotaMB() * ONE_MEG < availableMemory )
    availableMemory = myTdb().memoryQuotaMB() * ONE_MEG ;

  ULng32 minMemQuotaMB = myTdb().isPossibleMultipleCalls() ?
    myTdb().memoryQuotaMB() : 0 ;

  // in case we recreate, delete the old objects (incl. old scratch file)
  if ( cluster_ ) delete cluster_;
  if ( clusterDb_ ) delete clusterDb_;

  ULng32 minB4Chk = myTdb().getBmoMinMemBeforePressureCheck() * ONE_MEG;

  clusterDb_ = 
    new(heap_) ClusterDB(ClusterDB::SEQUENCE_OLAP,
			 myTdb().OLAPBufferSize_,
			 NULL, // workAtp_,
			 tdb.getExplainNodeId(),
			 0, // ... hashJoinTdb().extRightRowAtpIndex1_,
			 0, // ... hashJoinTdb().extRightRowAtpIndex2_,
			 NULL, // ... rightSearchExpr_,
			 NULL, // ... buckets_,
			 1, // bucketCount_ - must be > 0
			 availableMemory,
			 memMonitor,
			 myTdb().pressureThreshold_,
			 getGlobals()->castToExExeStmtGlobals(),
			 &rc_, 
			 myTdb().isNoOverflow(),
			 FALSE, /*isPartialGroupBy*/
			 0, // ... minBuffersToFlush_,
			 0, // ... numInBatch_,
			 
			 myTdb().forceOverflowEvery(),
			 0, // ... forceHashLoopAfterNumBuffers()
			 0, // ... forceClusterSplitAfterMB(),

			 // first time it's uninitialized - would setup later!
			 ioEventHandler_, // set up at registerSubtasks
			 this, // the calling tcb
			 myTdb().scratchThresholdPct_,
			 
			 myTdb().logDiagnostics(),
			 FALSE, // bufferedWrites(),
			 TRUE, // No early overflow based on cmp hints

			 myTdb().memoryQuotaMB(),
			 minMemQuotaMB,
			 minB4Chk, // BmoMinMemBeforePressureCheck
			 // next 4 are for early overflow (not used for OLAP)
			 0,
			 0,
			 0,
			 0,

			 0,    // ... Hash-Table not resizable
			 NULL // getStatsEntry()
			 );

  ex_assert( clusterDb_ , "No memory available for OLAP Operator");

  clusterDb_->setScratchIOVectorSize(myTdb().scratchIOVectorSize());
  switch(myTdb().getOverFlowMode())
  {
    case SQLCLI_OFM_SSD_TYPE: 
      clusterDb_->setScratchOverflowMode(SCRATCH_SSD);
      break;
    case SQLCLI_OFM_MMAP_TYPE: 
      clusterDb_->setScratchOverflowMode(SCRATCH_MMAP);
      break;
    default:
    case SQLCLI_OFM_DISK_TYPE:
      clusterDb_->setScratchOverflowMode(SCRATCH_DISK);
      break;
  }
  clusterDb_->setBMOMaxMemThresholdMB(myTdb().getBMOMaxMemThresholdMB());

  cluster_ = new(heap_) Cluster(Cluster::IN_MEMORY,
				       clusterDb_,
				       NULL, // ... &buckets_[bucketIdx], 
				       0, // ... bucketsPerCluster,
				       myTdb().recLen_, // Row Length
                                       false,
                                       false,
				       0, // ... extRightRowAtpIndex1_,
				       FALSE, // Use as an "outer cluster"
				       FALSE, // ... no bit map
				       NULL, // next cluster,
				       &rc_);
  
  ex_assert( cluster_ , "No memory available for OLAP Operator");
}

void ExSequenceTcb::updateDiagsArea(ex_queue_entry * centry)
{
    if (centry->getDiagsArea()) 
    {
      if (workAtp_->getDiagsArea())
      {     
        workAtp_->getDiagsArea()->mergeAfter(*centry->getDiagsArea());
      }
      else
      {
        ComDiagsArea * da = centry->getDiagsArea();
        workAtp_->setDiagsArea(da);
        da->incrRefCount();
        centry->setDiagsArea(0);
      }
    }
}

void ExSequenceTcb::updateDiagsArea(ComDiagsArea *da)
{
    if (workAtp_->getDiagsArea())
    {     
      workAtp_->getDiagsArea()->mergeAfter(*da);
    }
    else
    {
      workAtp_->setDiagsArea(da);
      da->incrRefCount();
    }
}

void ExSequenceTcb::updateDiagsArea(  ExeErrorCode rc_)
{                   
    ComDiagsArea *da = workAtp_->getDiagsArea();
    if(!da) 
    {
      da = ComDiagsArea::allocate(heap_);
      workAtp_->setDiagsArea(da);
    }
    if (!da->contains((Lng32) -rc_))
    {
      *da << DgSqlCode(-rc_);
      *da << DgString0("Sequence Operator Error occurred.");
    }
}
//
// Constructor and destructor private state
//
ExSequencePrivateState::ExSequencePrivateState
()
{
  matchCount_ = 0;
  step_ = ExSequenceTcb::ExSeq_EMPTY;
}

ExSequencePrivateState::~ExSequencePrivateState()
{
};

ex_tcb_private_state * ExSequencePrivateState::allocate_new
(const ex_tcb *tcb)
{
  return new(((ex_tcb*)tcb)->getSpace()) 
    ExSequencePrivateState();
};


////////////////////////////////////////////////////////////////////////
// Redefine virtual method allocatePstates, to be used by dynamic queue
// resizing, as well as the initial queue construction.
////////////////////////////////////////////////////////////////////////
ex_tcb_private_state * ExSequenceTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExSequencePrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}
