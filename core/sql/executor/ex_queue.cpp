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
* File:         ex_queue.c
* Description:  Methods for class ex_queue ex_queue_entry atp_struct
*               
*               
* Created:      5/3/94
* 
* Language:     C++
* Status:       Experimental
*
*
*
*
******************************************************************************
*/

#include  "ex_stdh.h"
#include "stdlib.h"

#include "ComTdb.h"     // For cancel/error injection testing.
#include "ex_tcb.h"     // For cancel/error injection testing.
#include "ex_error.h"   // For cancel/error injection testing.


ex_queue::ex_queue(const queue_type    type,
		   queue_index         initialSize,
                   ex_cri_desc *       cri_desc,
                   CollHeap *          space,
                   enum queueAllocType allocType) :
  //ExGod(space),
  upDown_(type), size_(initialSize),
  numTuples_(cri_desc->noTuples()),
  queue_(NULL),
  resizePoints_(0),
  queueWasFull_(FALSE),
  needsResize_(FALSE),
  resizeLimit_(0),
  needsPstates_(FALSE),
  needsAtps_(FALSE)
{
  ULng32      i;
  
  // make size a power of 2 and greater than 1.
  //
  size_ = roundUp2Power(size_);

  ex_assert(size_ > 1, "invalid queue size");

  ex_assert((space != 0), "Must pass in a valid space pointer.");

  queue_ = (ex_queue_entry *)(space->allocateMemory(size_ *
                                                  sizeof(ex_queue_entry)));

            // Since code should tolerate head_ and tail_ wrapping 
            // around, initializing them to these values insures that 
            // this assumption is always tested in our developer regressions.
  head_ = UINT_MAX - 1;
  tail_ = UINT_MAX - 1;
  mask_ = size_ - 1;
  criDesc_ = cri_desc;
  atpsAllocated_  = FALSE;
  insertSubtask_  = NULL;
  unblockSubtask_ = NULL;
  cancelSubtask_  = NULL;
  // BertBert VV
  nextSubtask_    = NULL;
  // BertBert ^^
  resizeSubtask_  = NULL;

  for(i = 0; i<size_; i++)
    {
      // Initialize private state pointer
      queue_[i].pstate = NULL;
      
      // initialize the public state
      queue_[i].initializeState(upDown_);
      
      // Initialize the pointer to the atp_struct
      queue_[i].atp_ = NULL;
    }
  
  // allocate space for the array of tuple pointers if required to do so
  if(allocType == ALLOC_ATP ||
     allocType == ALLOC_DEFAULT && upDown_ == UP_QUEUE)
    allocateAtps(space);
}

ex_queue::~ex_queue()
{
  if(atpsAllocated_)
    deallocateAtps();

  deallocatePstate();
}

// some weight factors for state transitions of queues
#define SUB_EMPTY_EMPTY_TRANSITION 2
#define ADD_EMPTY_FULL_TRANSITION  3
#define ADD_FULL_EMPTY_TRANSITION  3
#define SUB_FULL_FULL_TRANSITION   1

void ex_queue::handleFullQueue()
{
  // if head was the same as the tail that meant that the queue
  // was full (blocked), notify the task that is waiting for the
  // queue to become unblocked, but do it first!!
  head_++;  // ok to wrap around
  unblockSubtask_->schedule();
  
  // logic for dynamic resizing of queues
  
  if (queueWasFull_)
    {
      // the queue went from full to full without reaching
      // the empty state, decrement the count of empty/full
      // transitions
      if (resizePoints_ > SUB_FULL_FULL_TRANSITION)
	resizePoints_ -= SUB_FULL_FULL_TRANSITION;
    }
  else
    {
      // the query became empty in the recent past, increment
      // empty/full transitions
      resizePoints_ += ADD_EMPTY_FULL_TRANSITION;
      if (resizePoints_ >= resizeLimit_ && resizeSubtask_)
	{
	  // We have reached the resize limit and should resize.
	  needsResize_ = TRUE;
	  resizeSubtask_->schedule();
	}
      queueWasFull_ = TRUE;
    } // end of logic for dynamic resizing of queues
  // at this point the queue was full and queueWasFull_ is TRUE
}

void ex_queue::handleEmptyQueue()
{
  if (queueWasFull_)
    {
      // the query became full in the recent past, increment
      // empty/full transitions
      resizePoints_ += ADD_FULL_EMPTY_TRANSITION;
      if (resizePoints_ >= resizeLimit_ && resizeSubtask_)
	{
	  needsResize_ = TRUE;
	  resizeSubtask_->schedule();
	}
      queueWasFull_ = FALSE;
    }
  else
    {
      // the queue went from empty to empty without reaching
      // the full state, decrement the count of empty/full
      // transitions
      if (resizePoints_ > SUB_EMPTY_EMPTY_TRANSITION)
	resizePoints_ -= SUB_EMPTY_EMPTY_TRANSITION;
    }
  // at this point the queue is empty and queueWasFull_ is FALSE
}

#ifdef _DEBUG		
void ex_queue::removeHead()
{
  // make sure the queue is not empty
  ExQueueAssert(!isEmpty(), "ex_queue::removeHead() on an empty queue");
  
  ex_queue_entry *headEntry = &queue_[head_ & mask_]; //(size_ - 1)];
  
#ifdef _DEBUG
  logRemoveHead();
#endif
  
  if (atpsAllocated_ && headEntry->getAtp())
    headEntry->getAtp()->release();
  
  if (isFull())
    handleFullQueue();
  else
    {
      head_++;  // ok to wrap around
      
      // logic for dynamic resizing of queues
      if (isEmpty())
	{
	  handleEmptyQueue();
	} // end of logic for dynamic resizing of queues
    }
} // ex_queue::removeHead()
#endif
	       
//soln 10-040111-2308 start
void ex_queue::deleteHeadEntry()
{
  // removes the head entry. Same as removeHead except that it doesn't 
  // any queue resizing logic.
  // To be used to remove head entries ONLY after a cancel request has been 
  // sent by the operator. This method removes head entries without initiating
  // the q resizing logic. This is used to handle special cases like cancelling
  // a request after a GET_N is satisfied.After this removal of entries from
  // the queue is just looking for a Q_NO_DATA. Hence these shouldnt be 
  // considered for qresizing as the query has been satisfied.

  // make sure the queue is not empty
  ExQueueAssert(!isEmpty(), "ex_queue::deleteHeadEntry() on an empty queue");

  ex_queue_entry *headEntry = &queue_[head_ & mask_]; //(size_ - 1)];
  //headEntry->initializeState(upDown_);

#ifdef _DEBUG
  logRemoveHead();
#endif
  if (atpsAllocated_ && headEntry->getAtp())
    {
      headEntry->getAtp()->release();
    }

  if (isFull())
    {
      head_++; //ok to wrap around
      unblockSubtask_->schedule();
    }
  else 
    head_++; //ok to wrap around

  // Since this is called in the cancel routine, set resize points to zero 
  // so that we do not increase the points.
  // Note that resize points are persistant across requests and hence it is 
  // essential to reset it to 0, so that resize points dont get accumulated 
  // across requests
  resizePoints_ = 0;

} // ex_queue::deleteHeadEntry()
//soln 10-040111-2308 end

void ex_queue_entry::passAtp(const ex_queue_entry *from)
{
  ExQueueAssert(atp_ == NULL || criDesc() == from->criDesc(),
         "incompatible queue entries");
  atp_ = from->atp_;
}

void ex_queue_entry::passAtp(atp_struct *from)
{
  ExQueueAssert(atp_ == NULL || criDesc() == from->getCriDesc(),
         "incompatible queue entries");
  atp_ = from;
}


NABoolean ex_queue::allocateAtps(CollHeap * space,
				 atp_struct * atps,
				 queue_index numNewAtps,
				 Int32 atpSize,
				 NABoolean failureIsFatal)
{
  queue_index      i;

  // remember that we need ATPs for this queue
  needsAtps_ = TRUE;

  // if the number of needed ATPs is not passed in, assume that all
  // queue entries need an ATP
  if (atps == NULL)
    {
      // initially try to get a whole array of ATP's rather than one
      // at a time We are sent back the size of the atp struct since
      // it also allocates space for number of tuples per criDesc.
      numNewAtps = size_;
      atps = allocateAtpArray( criDesc_, numNewAtps, &atpSize, space, failureIsFatal);
    }

  // check whether the caller provided the ATPs or whether we were
  // successful in allocating them as an array
  if (atps)
    {
      ex_assert(atpSize >= sizeof(atp_struct),
		"Invalid ATP size passed to ex_queue::allocateAtps()");
      for(i = 0; i < size_; i++)
	{
	  if (queue_[i].atp_ == NULL)
	    {
	      ex_assert(numNewAtps != 0,
			"There are more empty queue entries than new ATPs");
	      numNewAtps--;
	      queue_[i].atp_ = atps;
	      atps = (atp_struct *) ((char *)atps + atpSize);
	    }
	}
    }
  else 
    // Didn't get enough space to get all of them at one shot so do
    // it one at a time.
  // Allocate space for the tuples in each atp
  // Allocate the atp if it has not been allocated yet
  for(i = 0; i<size_; i++)
  {
	if (queue_[i].atp_ == NULL)
	  {
	    queue_[i].atp_ = allocateAtpArray(criDesc_, 1, &atpSize, space, failureIsFatal);
	    if (!failureIsFatal && queue_[i].atp_ == NULL)
	      return FALSE; // let user handle the failure
	    ex_assert(queue_[i].atp_,"Memory Allocation Fatal Failure Expected but Jump Buffer does not seem to be set!");
	  }
  }
  atpsAllocated_ = TRUE;
  return TRUE;
}

void ex_queue::allocatePstate(ex_tcb_private_state *p, ex_tcb * tcb)
{
  queue_index      i;

  // Allocate space for the private state in each queue entry
  for(i = 0; i<size_; i++)
    {
      if (queue_[i].pstate == NULL)
	queue_[i].pstate = p->allocate_new(tcb);
    }

  needsPstates_ = TRUE;
}

NABoolean ex_queue::allocatePstate(ex_tcb * tcb,
			      ex_tcb_private_state *pstates,
			      queue_index numNewPstates,
			      Lng32 pstateLength,
			      NABoolean failureIsFatal)
{
  queue_index i;
  ex_tcb_private_state *actualPstates = pstates;
  Lng32 actualNumPstates = numNewPstates;
  Lng32 actualPstateLength = pstateLength;

  if (actualPstates == NULL)
    {
      actualNumPstates = size_;
      // NOTE: actualNumPstates may be changed (reduced) by this call
      actualPstates = tcb->allocatePstates(actualNumPstates,
					   actualPstateLength);
      if (!failureIsFatal && actualNumPstates == 0)
	return FALSE; // out of memory and user wants to handle this

      ex_assert(actualNumPstates > 0, "Unable to allocate even one pstate");
    }
  else
    {
      // As Lenin said: trusting is good, checking is better
      ex_assert(actualNumPstates > 0,"Can't preallocate 0 pstates");
      ex_assert(actualPstateLength >= sizeof(ex_tcb_private_state),
		"Pstate size is too small");
    }

  // Allocate space for the private state in each queue entry
  for(i=0; i<size_; i++)
    {
      if (queue_[i].pstate == NULL)
	{
	  // need a pstate
	  if (actualNumPstates <= 0)
	    {
	      // the array doesn't have enough pstates in it, allocate
	      // the entries one by one
	      actualNumPstates = 1;
	      actualPstates =
		tcb->allocatePstates(actualNumPstates, actualPstateLength);
	      if (!failureIsFatal && actualNumPstates == 0)
		return FALSE; // out of memory and user wants to handle this
	      ex_assert(actualNumPstates,"Unable to allocate a pstate");
	    }

	  // let the current queue entry point to the first entry of
	  // the array and advance the array pointer to the next element
	  queue_[i].pstate = actualPstates;
	  actualPstates = (ex_tcb_private_state *)
	    (((char *)actualPstates) + actualPstateLength);
	  actualNumPstates--;
	}
    }

  needsPstates_ = TRUE;
  return TRUE;
}

void ex_queue::deallocateAtps()
{
  queue_index      i;

  for(i = 0; i<size_; i++)
  {
    // release all the tupps before deallocating the array of tupps
    if (queue_[i].atp_)
      queue_[i].atp_->release();

    // de-allocate array of pointers to tuples
    //::deallocateAtp(queue_[i].atp_, collHeap());
    queue_[i].atp_ = 0;
  }
  atpsAllocated_ = FALSE;
}

void ex_queue::deallocatePstate()
{
  // don't explicitly delete the pstate objects since they came
  // from the space and since they may have been allocated as
  // arrays
  needsPstates_ = FALSE;
}

queue_index ex_queue::resize(ex_tcb *    tcb,
			     queue_index newSize)
{
  queue_index          sizeDelta;
  Space                *space = tcb->getSpace();
  ex_queue_entry       *newQueue;
  queue_index          newMask;
  Int32                  queueWasFull = isFull();
  NABoolean            needAtps = needsAtps();
  Int32                  atpSize = 0;
  atp_struct           *atps = NULL;
  queue_index          numAllocatedAtps = 0;
  ex_tcb_private_state *pstates = NULL;
  Lng32                 numAllocatedPstates = 0;
  Lng32                 pstateLength = 0;
  queue_index          qi;
  queue_index          lastEntryToCopy;
  ex_queue_entry       *oldQueue = queue_;
  const queue_index    oldSize = size_;
  const queue_index    oldMask = mask_;
  
  // the new size must be a power of 2, round up to the next power of 2
  queue_index p2 = size_;
  while (p2 < newSize && p2 != 0)
    p2 = p2 << 1;
  // p2 is now a power of 2 that is >= newSize, except that
  // it is 0 in some unsupported cases, like size_ being 0 or newSize
  // being too large to be rounded up to a power of 2
  newSize = p2;

  // can't resize to a smaller size
  if (newSize <= size_)
    return size_;

  sizeDelta = newSize - size_;
  newMask   = newSize - 1;

  // try to allocate the needed memory first and give up if that's not
  // possible

  // allocate new array of ex_queue_entry structs
  newQueue = (ex_queue_entry *) (space->allocateMemory(
       newSize * sizeof(ex_queue_entry),FALSE));
  if (newQueue == NULL)
    return size_; // out of memory

  // allocate array of ATPs if needed
  if (needAtps)
    {
      atps = allocateAtpArray(criDesc_, sizeDelta, &atpSize, space, FALSE);

      // for now, give up if it wasn't possible to allocate the ATPs in
      // one piece (one could try to allocate them one by one)
      if (atps == NULL)
	return size_; 

      numAllocatedAtps = sizeDelta;
    }

  // allocate an array of pstate objects if needed
  if (needsPstates_)
    {
      numAllocatedPstates = sizeDelta;
      pstates = tcb->allocatePstates(numAllocatedPstates,pstateLength);
      if (pstates == NULL)
	{
	  // either the TCB doesn't support the allocatePstates()
	  // method yet (it would be a dumb thing for a TCB not to
	  // support this AND to call the resize method) or we don't
	  // have enough memory. Give up without changing the size.
	  return size_;
	}
    }

  // at this point we have allocated everything we need, so we
  // should be in good shape from now on

  // initialize the new queue
  for(qi = 0; qi < newSize; qi++)
    {
      // Initialize private state pointer
      newQueue[qi].pstate = NULL;
      
      // initialize the public state
      newQueue[qi].initializeState(upDown_);
      
      // Initialize the pointer to the atp_struct
      newQueue[qi].atp_ = NULL;
    }

  // calculate last entry to copy (the addition here may wrap around
  // such that <lastEntryToCopy> is actually smaller than <head_>)
  lastEntryToCopy = head_ + size_;

  // Copy all queue entries (whether they are in use or not) over to
  // the new queue. Do this in such a way that the used entries move
  // to the new queue entry that corresponds to their index. Note
  // that we will start copying the used entries (if any) first, and
  // that those are the first entries until (but not including) the
  // entry where qi is equal to <tail_>.
  for (qi = head_; qi != lastEntryToCopy; qi++)
    {
      newQueue[qi & newMask] = queue_[qi & mask_];
    }

  // At this point, <size_> elements of the new queue are initialized
  // with actual queue entries, while <sizeDelta> entries may still
  // need ATPs and/or pstates (if applicable). These sizeDelta entries
  // will be taken care of next by using the regular allocation methods
  // for new queues. Note that the ATPs and PSTATEs of the remaining
  // entries may or may not be contiguous.

  // reset counter that counts empty/full transitions
  resizePoints_ = 0;
  needsResize_ = FALSE;

  // set the data member to point to the newly allocated structures
  queue_ = newQueue;
  size_  = newSize;
  mask_  = newMask;

  NABoolean finalAllocSucceeded = TRUE;

  // initialize the uninitialized ATPs if required to do so
  if (needAtps)
    finalAllocSucceeded = allocateAtps(space,
				       atps,
				       numAllocatedAtps,
				       atpSize,
				       FALSE);

  // allocate PSTATEs for the uninitialized queue entries if this
  // queue needs PSTATEs
  if (needsPstates_ && finalAllocSucceeded)
    {
      finalAllocSucceeded = allocatePstate(tcb,
					   pstates,
					   numAllocatedPstates,
					   pstateLength,
					   FALSE);
    }

  if (finalAllocSucceeded)
    {
      // schedule the unblock subtask of the queue if the queue was full
      // before the resize and now is no longer full (a deadlock could
      // occur if we wouldn't schedule the unblock task for a full to
      // not full transition of a queue)
      if (queueWasFull)
	unblockSubtask_->schedule();
    }
  else
    {
      // We failed during allocation of ATPs or PStates. Undo the switch
      // to the new, resized queue and restore the queue to its old state.
      queue_ = oldQueue;
      size_  = oldSize;
      mask_  = oldMask;
    }

  return size_;
}

void ex_queue::cancelRequestWithParentIndex(queue_index pindex)
{
  queue_index i = head_;

  // search through all existing entries for the given parent index
  while (!isVacant(i))
    {
      // if this entry has the given parent index, cancel it
      if (queue_[i&mask_].downState.parentIndex == pindex)
        cancelRequest(i);
      i++;
    }
}

// Cancel all requests with a parent index in the range from startPI to endPI (inclusive)
void ex_queue::cancelRequestWithParentIndexRange(queue_index startPI, queue_index endPI)
{
  queue_index i = head_;

  // search through all existing entries for the parent indexes within the range
  if(endPI >= startPI) {
    while (!isVacant(i))
      {
        // if this entry is within the range, cancel it
        if (queue_[i&mask_].downState.parentIndex >= startPI &&
            queue_[i&mask_].downState.parentIndex <= endPI)
          cancelRequest(i);
        i++;
      }
  } else if(startPI > endPI) {
    // If endPI has wrapped around, the range also wraps.
    while (!isVacant(i))
      {
        // if this entry is within the range, cancel it
        if (queue_[i&mask_].downState.parentIndex >= startPI ||
            queue_[i&mask_].downState.parentIndex <= endPI)
          cancelRequest(i);
        i++;
      }
  }
}

// used to initiliaze an empty queue entry
void ex_queue_entry::initializeState(const ex_queue::queue_type type)
{
  switch(type)
  {
  case ex_queue::UP_QUEUE:
    upState.status = ex_queue::Q_INVALID;
    break;

  case ex_queue::DOWN_QUEUE:
    downState.request = ex_queue::GET_EMPTY;
    downState.requestValue = 0;
    // BertBert VV
    downState.numGetNextsIssued = 0;
    // BertBert ^^
    break;
  }
}

const char * NodeTypeToString(ComTdb::ex_node_type nodeType)
{
  const char * tdbName;
  
  switch (nodeType)
    {
    case ComTdb::ex_DDL:            
      tdbName = "ex_ddl";
      break;

    case ComTdb::ex_DESCRIBE:       
      tdbName = "ex_describe";
      break;

    case ComTdb::ex_EXE_UTIL:
      tdbName = "ExExeUtil";
      break;

    case ComTdb::ex_ROOT:               
      tdbName = "ex_root";
      break;

    case ComTdb::ex_ONLJ:               
      tdbName = "ex_onlj";
      break;

    case ComTdb::ex_MJ:         
      tdbName = "ex_mj";
      break;

    case ComTdb::ex_HASHJ:
      tdbName = "ex_hashj";
      break;

    case ComTdb::ex_HASH_GRBY:
      tdbName = "ex_hash_grby";
      break;

    case ComTdb::ex_LOCK:            
      tdbName = "ExLock";
      break;

    case ComTdb::ex_SORT:
      tdbName = "ExSort";
      break;

    case ComTdb::ex_SORT_GRBY:  
      tdbName = "ex_sort_grby";
      break;

    case ComTdb::ex_SEQUENCE_FUNCTION:
      tdbName = "ex_sequence_function";
      break;

    case ComTdb::ex_TRANSACTION:
      tdbName = "ExTrans";
      break;

    case ComTdb::ex_SET_TIMEOUT:   
      tdbName = "ExTimeout";
      break;

    case ComTdb::ex_UNION:
      tdbName = "ex_union";
      break;

    case ComTdb::ex_TUPLE:          
       tdbName = "ex_tuple";
      break;

   case ComTdb::ex_SPLIT_TOP:      
      tdbName = "ex_split_top";
      break;

    case ComTdb::ex_SPLIT_BOTTOM:   
      tdbName = "ex_split_bottom";
      break;

    case ComTdb::ex_PARTN_ACCESS:
      tdbName = "ex_partn_access";
      break;

    case ComTdb::ex_SEND_TOP:       
      tdbName = "ex_send_top";
      break;

    case ComTdb::ex_SEND_BOTTOM:    
      tdbName = "ex_send_bottom";
      break;

    case ComTdb::ex_EXPLAIN:
      tdbName = "exExplain";
      break;

    case ComTdb:: ex_TUPLE_FLOW:
      tdbName = "ExTupleFlow";
      break;
    case ComTdb::ex_FAST_EXTRACT:
      tdbName = "ExFastExtract";
      break;
    case ComTdb::ex_UDR:		
      tdbName = "ExUdr";
      break;

    case ComTdb::ex_PROBE_CACHE:		
      tdbName = "ExProbeCache";
      break;

    case ComTdb::ex_HDFS_SCAN:
      tdbName = "ExHdfsScan";
      break;

    case ComTdb::ex_ARQ_WNR_INSERT:
      tdbName = "ExExeUtilAqrWnrInsert";
      break;

    default:
      tdbName = "NULL";
     
    }

  return tdbName; 
}

void ex_queue::injectErrorOrCancel()
{
#ifdef _DEBUG
  ex_queue_entry *qe = getQueueEntry(tail_-1);

  // DO the ol' switcheroo, but not every time.
  ULng32 freq = insertSubtask_->getTcb()->getGlobals()->getInjectErrorAtQueue();
  if (freq == 0)
      return;
  if (upDown_ == UP_QUEUE)
    {
       if ((rand() & (freq-1)) != 0)
         return;
        NABoolean needsError = FALSE;
        switch (qe->upState.status)
        {
        case Q_OK_MMORE:
          {
            needsError = TRUE;
            qe->upState.status = Q_SQLERROR;
            cerr << "Converting a Q_OK_MMORE to a Q_SQLERROR, from "
                 << NodeTypeToString(unblockSubtask_->getTcb()->getNodeType())
                 << "(" << unblockSubtask_->getTcb() << ")" 
                 << " to "
                 << NodeTypeToString(insertSubtask_->getTcb()->getNodeType())
                 << "(" << insertSubtask_->getTcb() << ")" 
                 << endl;
            break;
          }
        case Q_NO_DATA:
          if (!isFull())
            {
              needsError = TRUE;
              qe->upState.status = Q_SQLERROR;
              ex_queue_entry *newQNODATA = getTailEntry();
              newQNODATA->upState = qe->upState;
              newQNODATA->upState.status  = Q_NO_DATA;
              newQNODATA->getAtp()->copyAtp(qe->getAtp());
              tail_++;
              cerr << "Injecting a Q_SQLERROR before a Q_NO_DATA, from "
                 << NodeTypeToString(unblockSubtask_->getTcb()->getNodeType())
                 << "(" << unblockSubtask_->getTcb() << ")"
                 << " to "
                 << NodeTypeToString(insertSubtask_->getTcb()->getNodeType())
                 << "(" << insertSubtask_->getTcb() << ")"
                 << endl;
            }
          break;
        default:
            break;
        }
        if (needsError)
          {
            ComDiagsArea * da = qe->getDiagsArea();
            if (!da)
              da = ComDiagsArea::allocate(insertSubtask_->getTcb()->getHeap());
            else
              da = da->copy();
            qe->setDiagsAreax(da);

            *da << DgSqlCode(-EXE_ERROR_INJECTED)
                << DgString0(__FILE__)
                << DgInt0(__LINE__);
          }
    }
#endif

  return;
}

void ex_queue::logHeader()
{
  if (upDown_ == UP_QUEUE)
    {
      cerr
         << NodeTypeToString(unblockSubtask_->getTcb()->getNodeType())
         << "(" << unblockSubtask_->getTcb() << ")"
         << " up to "
         << NodeTypeToString(insertSubtask_->getTcb()->getNodeType())
         << "(" << insertSubtask_->getTcb() << ")"
         ;
    }
  else
    {
      cerr << NodeTypeToString(unblockSubtask_->getTcb()->getNodeType())
           << "(" << unblockSubtask_->getTcb() << ")"
           << " down to "
           << NodeTypeToString(insertSubtask_->getTcb()->getNodeType())
           << "(" << insertSubtask_->getTcb() << ")" 
           ;
    }
}

void ex_queue::logRemoveHead()
{
#if 0  

    ex_queue_entry *qe = &queue_[head_ & mask_];

    logHeader();

    cerr << "remove ";

 if (upDown_ == UP_QUEUE)
    {
      switch (qe->upState.status)
        {
        case Q_OK_MMORE:
          cerr << "Q_OK_MMORE";
          break;
        case Q_SQLERROR:
          cerr << "Q_SQLERROR";
          break;
        case Q_NO_DATA:
          cerr << "Q_NO_DATA";
          break;
        default:
          cerr << "unknown";
          break;
        }
      cerr << ", pi " << qe->upState.parentIndex;
    }
  else
    {
      switch (qe->downState.request)
        {
        case GET_ALL:
          cerr << "GET_ALL";
          break;
        case GET_N:
          cerr << "GET_N";
          break;
        case GET_EOD:
          cerr << "GET_EOD";
          break;
        case GET_NOMORE:
          cerr << "GET_NOMORE";
          break;
        default:
          cerr << "unknown";
          break;
        }
       cerr << ", pi " << qe->downState.parentIndex;
   }
  cerr << endl;
#endif
}

queue_index ex_queue::roundUp2Power(queue_index i)
{
  ULng32 count = 1;     
  queue_index s = i - 1;
  while (s > 1) {
    count++;
    s = s >> 1;
  };

  if (count > (sizeof(queue_index) * 8))
    s  = maxQueueSize;
  else
    s = (1 << count);
  return s;
}









