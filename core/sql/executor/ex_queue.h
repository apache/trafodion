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
#ifndef EX_QUEUE_H
#define EX_QUEUE_H

/* -*-C++-*-
******************************************************************************
*
* File:         ex_queue.h
* Description:  Classes and structures for queues and rows
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

#include "ExpAtp.h"
#include "ExScheduler.h"
#include "ex_god.h"
#include "ex_ex.h"

#ifdef _DEBUG
#define ExQueueAssert(a,b) ex_assert(a,b)
#else
#define ExQueueAssert(a,b)
#endif

//
// This file defines the classes and structures that implement queues 
// and queue entries.
//
// The classes defined in this file:
//
class ex_queue;       // queue of composite rows and row state
class ex_queue_entry; // an entry in the queue
class down_state;     // describes state of down-queue entry
class up_state;       // describes state of up-queue entry

// other classes referenced
class ComDiagsArea; // diagnostics area
class ex_tcb_private_state;

// queue_index is a handle into a queue entry
// Changing queue_index to int reduces the number of instructions required
// to access the queue vector. Shorts actually take more instructions.
#include "QueueIndex.h"

// maximum queue length
const queue_index maxQueueSize = 0xffffffff;

// -------------------------------------------------------------------------
// Queue
//
class ex_queue : public ExGod
{
public:
  enum queue_type {UP_QUEUE, DOWN_QUEUE};
  
  enum down_request
  {
    GET_N,        // return N matching rows
    GET_ALL,      // return all matching rows
    GET_NOMORE,   // get no more matching rows - Cancel request
    GET_EMPTY,    // empty entry - no request
    GET_EOD,      // this request type indicates that it is the end
                  // of input requests. Used when multiple input rows
                  // that are part of the same request, are sent to an 
                  // operator from 'top'.For example, insert...select request. 
                  // The select rows are input to insert operator until
                  // all selected rows have been retrieved. See
                  // ExTupleFlowTcb for details.
    
    // When multiple user rowset inserts are to be part of the same sidetree
    // insert request, then this indicates to eid that all rows 
    // have been sent for this particular request but sidetree insert is
    // still in progress and should not be committed.
    GET_EOD_NO_ST_COMMIT,   

    // BertBert VV
    // The GET_NEXT_N protocol is currently used for "cursors for delete access" because
    //  for such cursors, the application must have fetched all rows that have been deleted.
    //  Of course, this only works well if cursors stay open across transactions.
    //  The GET_NEXT_N protocol allows one to control accurately how many rows are returned
    //  (and deleted in the 'for delete access' case).
    // The GET_ALL protocol is used for regular cursors that stay open across transactions
    //  because it is assumed that such cursors don't lock.  If they do, they should use the
    //  GET_NEXT_N protocol.
    // Each GET_NEXT_N must be matched with a Q_GET_DONE.  it is the responsibility
    //  of each operator to guarantee that.
    // One must not wait for a Q_GET_DONE before issueing a subsequent GET_NEXT_N
    //  because GET_NEXT_N is cumulative (adds to requestValue).  However, none of the
    //  operators that support the GET_NEXT protocol have been upgraded to handle "asynchronous"
    //  GET_NEXTs. note that there is no synchronization in this queue class.
    GET_NEXT_N,   // Attempt to return N rows.  If there aren't N to return, return less.
                  // After returning N (or less or zero) rows, return the up_status Q_GET_DONE.
    GET_NEXT_N_MAYBE_WAIT,
                  // Attempt to return N rows.  If there are none to return, block waiting for
                  //  rows to appear.  If there are some rows to return, return them.
                  // After returning N (or less but not zero) rows, return the up-status Q_GET_DONE.
                  // Note that only a leaf EID operator can block waiting for rows to appear, and that
                  //  this only can happen if the access is for 'stream' access.  For the rest, 
                  //  GET_NEXT_N_MAYBE_WAIT is identical to GET_NEXT_N.
    GET_NEXT_0_MAYBE_WAIT,
                  // This command never returns any rows.  If rows could be returned, return the 
                  //  up-status Q_GET_DONE.  If no rows could be retured, block waiting for rows
                  //  to appear.  The command is used for streaming destructive selects on
                  //  partitioned tables to check which partition we can destructively select
                  //  from.

    GET_NEXT_N_SKIP,
    GET_NEXT_N_MAYBE_WAIT_SKIP,
    GET_N_RETURN_SKIPPED,
    // This setting is used specifically in the case of index joins. This indicates that the 
    // right child has skipped the record due to skip conflict access and this has to be added 
    // to the delta key list in the right child.. 
    // BertBert ^^
  }; 
   
  enum up_status
  {
    Q_NO_DATA,    // no more matching data found
    Q_OK_MMORE,   // matching row returned, maybe more coming
    Q_SQLERROR,   // sql error and (optionally) matching data
    Q_INVALID     // invalid state
    // BertBert VV
    ,
    Q_GET_DONE,   // Used for destructive scans (which always use the GET_NEXT_N protocol).
                  //  Indicates that a complete set of matching rows has
                  //  been returned and the client should re-issue GET_NEXT_N/GET_NEXT_N_MAYBE_WAIT 
                  //  to retrieve more rows.  The field matchNo in the up-state contains the total
                  //  number of rows requested that have been satisfied with this control tuple.
                  // Used for non-destructive scans (which always use the GET_ALL protocol).
                  //  It causes the DP2-root operator to reply with a buffer of accumulated rows,
                  //  eventhough the buffer might not be full yet.  In this case, the field matchNo
                  //  in tht up-state will be -1.
    Q_REC_SKIPPED, // Used specifically in the case of an index join. The right child of the join 
    // returns this to indicate that this record is skipped because it was locked and skip conflict
    // was used. 
    // BertBert ^^
   Q_STATS // added for runtime stats by Srinivas for handling empty SQL buffers
    // but with stats
  };

  enum queueAllocType // to allocate or not to allocate, that is the question
  {
    ALLOC_DEFAULT, // allocate for up queue, don't allocate for down queue
    ALLOC_ATP,     // allocate the ATPs for this queue
    ALLOC_NONE     // don't allocate the ATPs for this queue
  };

  // Constructor for a queue
    ex_queue(const queue_type    type,
	     queue_index         initialSize,
	     ex_cri_desc *       criDesc,
	     CollHeap *          space,
	     enum queueAllocType allocType = ALLOC_DEFAULT);

  
  // allocate ATPs for entries that don't have one already,
  // use pre-allocated ATPs if they are passed in
    NABoolean allocateAtps(CollHeap * space,
			   atp_struct * atps = NULL,
			   queue_index numNewAtps = 0,
			   Int32 atpSize = 0,
			   NABoolean failureIsFatal = TRUE);

  // allocate PSTATE objects for all queue entries, pass in a single
  // PSTATE as a sample (deprecated method, use the other method that
  // doesn't need the pstate to be passed in and that supports
  // preallocated pstates and allocation as an array)
    void allocatePstate(ex_tcb_private_state *p,
			ex_tcb * tcb);
  // allocate PSTATE objects for queue entries that don't have a
  // PSTATE yet, using an array of pre-allocated PSTATEs if passed in
    NABoolean allocatePstate(ex_tcb * tcb,
			     ex_tcb_private_state *pstates = NULL,
			     queue_index numNewPstates = 0,
			     Lng32 pstateLength = 0,
			     NABoolean failureIsFatal = TRUE);
  
    void deallocateAtps();
    void deallocatePstate();

  // does this queue need its own ATPs?
  Int32 needsAtps() const                               { return needsAtps_; }

  // resize a queue (including the ATPs and the pstates), return new size
  NABoolean needsResize() const                     { return needsResize_; }
  Lng32 getResizeLimit() const                { return (Lng32) resizeLimit_; }
  void setResizeLimit(Lng32 l)          { resizeLimit_ = (ULng32) l; }
    queue_index resize(ex_tcb *    tcb,
		       queue_index newSize);

    ex_cri_desc * getCriDesc();
  
  // destructor
    ~ex_queue();

  //
  // Insert into the queue methods
  //
  // The following two methods are called only by the node
  // that inserts into the queue. The parent inserts in the down
  // queue and the child inserts in the up queue
  //
  // gets address/index of empty queue_entry to modify it
  // (the entry is not yet inserted into the queue)
  //
  ex_queue_entry *  getTailEntry() const;
    queue_index       getTailIndex() const;
  
  // next queue_entry now becomes tail_
    void insert();
  
  //
  // Cancel a down request already inserted into the queue; cancel 
  // a request with a given value for parentIndex.
  //
  void cancelRequest();
    void cancelRequest(const queue_index i);
  void cancelRequestWithParentIndex(queue_index pindex);
  void cancelRequestWithParentIndexRange(queue_index startPI, queue_index endPI);

  //?johannes?
  void getNextNRequestInit(const Lng32 tuples);
  void getNextNRequest(const Lng32 tuples);

  void getNextNMaybeWaitRequestInit(const Lng32 tuples);
  void getNextNMaybeWaitRequest(const Lng32 tuples);

  void getNextNSkipRequestInit(const Lng32 tuples);
  void getNextNSkipRequest(const Lng32 tuples);

  void getNextNMaybeWaitSkipRequestInit(const Lng32 tuples);
  void getNextNMaybeWaitSkipRequest(const Lng32 tuples);

  void getNext0MaybeWaitRequest(void);
  void getNext0MaybeWaitRequestInit(void);
  //?johannes??

  //
  // Remove from the queue methods
  //
  // The following two methods are called only by the node
  // that reads and removes from the queue
  //
  // gets address of i-th queue-entry
  //
    ex_queue_entry *  getQueueEntry(const queue_index i) const;
    ex_queue_entry *  getHeadEntry() const;
    queue_index       getHeadIndex() const;

  // check whether a certain entry exists (used to loop through entries)
  inline Int32        entryExists(queue_index ix) const;

  void handleFullQueue();
  void handleEmptyQueue();

  // removes the head and it becomes empty;
#ifndef _DEBUG
inline
#endif
  void removeHead();

  // soln 10-040111-2308 start
  // removes the head entry. Same as removeHead except that it doesn't 
  // any queue resizing logic.
  // To be used to remove head entries ONLY after a cancel request has been
  // sent by the operator. This method removes head entries without initiating
  // the q resizing logic. This is used to handle special cases like cancelling
  // a request after a GET_N is satisfied.After this removal of entries from
  // the queue is just looking for a Q_NO_DATA. Hence these shouldnt be
  // considered for qresizing as the query has been satisfied.

  void deleteHeadEntry();
  // soln 10-040111-2308 end

  // queue the size of the queue and the number of entries in it.
    queue_index    getSize() const;  //allocated size
    queue_index inline getLength() const;  //occupied entries between 0 and size-1
    Int32 inline     isFull() const;  // returns true if queue is full
    Int32 inline     isEmpty() const;  // returns true if queue is full

  inline void setInsertSubtask(ExSubtask *insertSubtask)
                                       { insertSubtask_ = insertSubtask; }
  inline void setUnblockSubtask(ExSubtask *unblockSubtask)
                                     { unblockSubtask_ = unblockSubtask; }
  void setCancelSubtask(ExSubtask *cancelSubtask)
  { ExQueueAssert(upDown_ == DOWN_QUEUE,""); cancelSubtask_ = cancelSubtask; }
  inline void setResizeSubtask(ExSubtask *resizeSubtask)
                                       { resizeSubtask_ = resizeSubtask; }

  void setNextSubtask(ExSubtask *nextSubtask)
  { ExQueueAssert(upDown_ == DOWN_QUEUE,""); nextSubtask_ = nextSubtask; }

  static queue_index roundUp2Power(queue_index i);

private:

  const queue_type   upDown_;  
  queue_index        size_;
  queue_index        head_;
  queue_index        tail_;

  // Using a bit mask is faster than masking with (size_ - 1).
  //
  queue_index        mask_;

  const unsigned short  numTuples_;  // number of tuple pointers in each atp

  // queue cri descriptors
  ex_cri_desc *   criDesc_;

  // are the ATPs allocated and owned by the queue?
  NABoolean atpsAllocated_;

  ex_queue_entry * queue_;           // pointer to (array of) queue entries
 
  // Subtasks associated with the queue
  ExSubtask *insertSubtask_;
  ExSubtask *unblockSubtask_;
  ExSubtask *cancelSubtask_;
  ExSubtask *resizeSubtask_;

  // Data members for dynamic resizing of queues: resizePoints_
  // keeps a weighted measure based on empty/full transitions
  // of how much the queue needs resizing. queueWasFull_ indicates
  // the previous state of the queue (empty or full). Once
  // resizePoints_ reaches resizeLimit_ it's time to trigger the
  // resize subtask of the queue.
  ULng32 resizePoints_;
  NABoolean     queueWasFull_;
  NABoolean     needsResize_;
  ULng32 resizeLimit_;

  // indiciates whether this queue needs ATPs and PSTATEs to be allocated
  NABoolean needsPstates_;
  NABoolean needsAtps_;

  //?johannes?
  ExSubtask *nextSubtask_;
  //?johannes??

  // is queue entry i vacant?
  inline Int32 isVacant(const queue_index i) const;

  void injectErrorOrCancel();

  // for debuggin'

 
	void logHeader();
 
    void logCancel(queue_index i)
    {
    #if 0  
      logHeader();
      cerr << " cancel " << i << " (" << (i  & (size_-1)) << ")" << endl;
    #endif
    }


    void logInsert()
    {
    #if 0  
      ex_queue_entry *qe = &queue_[tail_ & (size_ - 1)];

      logHeader();
      cerr << " insert ";

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

    void logRemoveHead();  
  
};  // ex_queue


// -------------------------------------------------------------------------
//
// Queue entry
//
// A queue entry encapsulates three things.  The first describes
// the state of an entry, represented as an instance of class down_state
// for down queues, and up_state for up queues.  The second is a pointer
// to a composite record instance.  The third is a structure where the
// reader of the queue can keep state associated with this input record. 
// The type/class of the record pointed to by the the second
// pointer depends on the type of node that is reading this queue.
//

class down_state    // public down status
{
public:
  ex_queue::down_request  request;

  // BertBert VV
  // a value that is associated with 'request'.
  // For GET_N, this is the number of rows that is requested.
  // For GET_NEXT_N, this is the total number of rows
  //  requested across all GET_NEXT_Ns received so far (i.e. accumulative)
  Lng32                   requestValue;

  // A count of the number of GET_NEXT_Ns issued by the parent.
  Lng32                   numGetNextsIssued;
  // BertBert ^^
  
  queue_index             parentIndex;

  inline NABoolean operator==(const down_state &other) const
  {
    return (request      == other.request) &&
           (requestValue == other.requestValue) &&
           (parentIndex  == other.parentIndex) &&
           (numGetNextsIssued  == other.numGetNextsIssued);
  };
};

// Some special values for a matchNo.
const Lng32 OverflowedMatchNo = -1;
const Lng32 GetDoneNoRowsMatchNo = -2;

class up_state    // public up state
{
friend class SqlBuffer;
friend class SqlBufferOlt;
friend class SqlBufferOltSmall;

public:
  inline NABoolean operator==(const up_state &other) const
  {
    return (parentIndex  == other.parentIndex) &&
           (status       == other.status);
  };

  queue_index      downIndex;    // replying to this particular down request
  queue_index      parentIndex;  // a copy of parent_index in the down state
  ex_queue::up_status    status; // type of entry (end-of-data, error, etc.)

  // accessors
  inline Lng32 getMatchNo(void) const;
  inline NABoolean getDoneNoRowsMatchNo(void) const;

  // mutators
  inline Lng32 setMatchNo(Int64 n);
  inline void setDoneNoRowsMatchNo(void);

private:
  // matchNo is private to handle overflow situations.  There is only one
  // consumer of matchNo where there is any possibility that matchNo has 
  // overflowed - that is ex_partn_access's workUp (and the olt  variant),
  // where matchNo is being used to return # of rows affected by an 
  // ins/upd/del.  See special logic there.
  // The only operators that create a value for matchNo that can overflow 
  // are the leaf operators in dp2.  We don't care about overflow except when
  // matchNo is used to count rows affected in an ins/upd/del.
  // 
  // BertBert VV
  // For Q_GET_DONE, this is the 'requestValue' that is matched by this
  //  control tuple.
  Lng32            matchNo;    // number of matches
  // BertBert ^^
};

class ex_queue_entry
{
  friend class ex_queue;  // class-key must be used when declaring a friend

 public:

  union   // anonymous
  {
    up_state    upState;
    down_state  downState;
  };

  ex_tcb_private_state * pstate;  // private (reader's) state
  
  
    inline ex_cri_desc *   criDesc() const;
  
    inline unsigned short  numTuples() const;
  
    inline tupp &          getTupp(Lng32 i) const;
  
     void            passAtp(const ex_queue_entry *from);
  
     void            passAtp(atp_struct *from);
  
    inline void            copyAtp(const ex_queue_entry *from);
  
    inline void            copyAtp(atp_struct *from);
  
    inline atp_struct *    getAtp() const;

  // 
  // Routines to get and set diagnostics area.
  //
  
    inline ComDiagsArea    *getDiagsArea() const;
  
    inline void            setDiagsArea(ComDiagsArea* diagsArea);

 private:

    void      initializeState (const ex_queue::queue_type type);

  // Used to check that an entry being inserted is intialized properly
  inline Int32 checkState(const ex_queue::queue_type type)
  {
    return (type == ex_queue::UP_QUEUE
            ? (upState.status != ex_queue::Q_INVALID)
            : (downState.request != ex_queue::GET_EMPTY));
  }


  atp_struct * atp_;    // a pointer to the composite row instance
};

//
// queue pair. A pair of queue pointers
//
struct ex_queue_pair
{
  ex_queue  *down;    // cris flowing down
  ex_queue  *up;      // cris flowing up
};


//////////////////////////////////////////////////////////////////////////////
//
//  Inline methods
//
//////////////////////////////////////////////////////////////////////////////

//
// for ex_queue_entry
//

inline ex_cri_desc * ex_queue_entry::criDesc() const
{
  return atp_->getCriDesc();
}

inline unsigned short ex_queue_entry::numTuples() const
{
  return criDesc()->noTuples();
}

inline atp_struct * ex_queue_entry::getAtp() const
{
        return atp_;
}

inline tupp & ex_queue_entry::getTupp(Lng32 i) const
{
  return atp_->getTupp(i);
}

inline void ex_queue_entry::copyAtp(const ex_queue_entry *from)
{
  atp_->copyAtp(from->getAtp());
}

inline void ex_queue_entry::copyAtp(atp_struct *from)
{
  atp_->copyAtp(from);
}

inline ComDiagsArea *ex_queue_entry::getDiagsArea() const
{
  return atp_->getDiagsArea();
}

inline void ex_queue_entry::setDiagsArea(ComDiagsArea* diagsArea)
{
  atp_->setDiagsArea(diagsArea);
}

/////////////////////////////////////////////////////////////////////////////
// Inline procedures -- class ex_queue

// queue is implemented in a circular array of size "size_".
//
// head_, tail_ and all variables of type queue_index are unsigned number that
// wrap around from max-value to 0. To find the entry in the array that holds
// them compute index%size_. Since size_ is always a power of 2, a faster
// way to compute modulo is index & (size_-1)
//
//  head_ points to the first occupied position - delete from head_
//  tail_ points to the next empty position - insert at tail_
//  head_ < tail_ (mod max unsigned number)
//
//  head_ = tail_ means queue is empty
//  head_ + size_ = tail_+1 means queue is full
//
//  tail_ - head_ (mod size_) is the number of occupied entries
//  [ mod max unsigned number ]

inline ex_queue_entry * ex_queue::getTailEntry() const
{
  return &queue_[tail_ & mask_]; //(size_ - 1)];
}

inline queue_index ex_queue::getTailIndex() const
{
  return tail_;
}

inline queue_index ex_queue::getLength() const
{
  return tail_ - head_;
  // note that this should even work for the very
  // unlikely case that tail_ < head_
}

inline Int32 ex_queue::isFull() const
{
  return (getLength() == mask_);

  //  return (head_ + size_) == (tail_ + (queue_index)1);
}

inline Int32 ex_queue::isEmpty() const
{
  return head_ == tail_;
}

inline Int32 ex_queue::isVacant(const queue_index i) const
{
  // index has to be between head_ (inclusive) and tail_ (exclusive)
  // to be not vacant if tail_ is less than head_ it means the queue
  // indices are wrapping around the values an unsigned int can hold
  
  if (head_<=tail_)
    return !((head_ <= i) && (i < tail_));
  else
    return !((head_ <= i) || (i < tail_));  
}

#ifndef _DEBUG
inline void ex_queue::removeHead()
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

inline void ex_queue::cancelRequest(const queue_index i)
{
  // It better be a down request
  ExQueueAssert(upDown_ == DOWN_QUEUE, "Can't cancel up queue entries");
  
  // check that the request has not been deleted
  if (!isVacant(i))
    {
#ifdef _DEBUG
      logCancel(i);
#endif
      queue_[i & mask_].downState.request = GET_NOMORE;

      // schedule the task that handles the cancel request
      cancelSubtask_->schedule();
    }
}

inline void ex_queue::cancelRequest()
{
  // It better be a down request
  ExQueueAssert(upDown_ == DOWN_QUEUE, "Can't cancel up queue entries");

#ifdef _DEBUG
  logCancel(head_);
#endif

  queue_[head_ & mask_].downState.request = GET_NOMORE;

  // schedule the task that handles the cancel request
  cancelSubtask_->schedule();
}


//?johannes?

inline void ex_queue::getNextNRequest( const Lng32 tuples)
{
  // It better be a down request
  ExQueueAssert(upDown_ == DOWN_QUEUE, "Can't put GET_NEXT_N into up queue entries");
  queue_[head_ & mask_].downState.request = GET_NEXT_N;

  // increment request value by number of tuples asked for
  queue_[head_ & mask_].downState.requestValue += tuples;

  // record that a new get next request has been issued.
  queue_[head_ & mask_].downState.numGetNextsIssued++;

  // schedule the task that handles the GET_NEXT request
  nextSubtask_->schedule();
}

inline void ex_queue::getNextNRequestInit( const Lng32 tuples)
{
  // It better be a down request
  ExQueueAssert(upDown_ == DOWN_QUEUE, "Can't put GET_NEXT_N into up queue entries");
  queue_[head_ & mask_].downState.request = GET_NEXT_N;

  // set request value by number of tuples asked for
  queue_[head_ & mask_].downState.requestValue = tuples;

  // record that a new get next request has been issued
  //    but reset the fields.
  queue_[head_ & mask_].downState.numGetNextsIssued = 1;

  // schedule the task that handles the GET_NEXT request
  nextSubtask_->schedule();
}


inline void ex_queue::getNextNMaybeWaitRequest( const Lng32 tuples)
{
  // It better be a down request
  ExQueueAssert(upDown_ == DOWN_QUEUE, "Can't put GET_NEXT_N_MAYBE_WAIT into up queue entries");
  queue_[head_ & mask_].downState.request = GET_NEXT_N_MAYBE_WAIT;

  //increment request value by number of tuples asked for
  queue_[head_ & mask_].downState.requestValue += tuples;

   //record that a new get next request has been issued.
  queue_[head_ & mask_].downState.numGetNextsIssued++;

  // schedule the task that handles the GET_NEXT request
  nextSubtask_->schedule();
}

inline void ex_queue::getNextNMaybeWaitRequestInit( const Lng32 tuples)
{
  // It better be a down request
  ExQueueAssert(upDown_ == DOWN_QUEUE, "Can't put GET_NEXT_N_MAYBE_WAIT into up queue entries");
  queue_[head_ & mask_].downState.request = GET_NEXT_N_MAYBE_WAIT;

  // set request value by number of tuples asked for
  queue_[head_ & mask_].downState.requestValue = tuples;

  // record that a new get next request has been issued.
  queue_[head_ & mask_].downState.numGetNextsIssued = 1;

  // schedule the task that handles the GET_NEXT request
  nextSubtask_->schedule();
}

inline void ex_queue::getNextNSkipRequest( const Lng32 tuples)
{
  // It better be a down request
  ExQueueAssert(upDown_ == DOWN_QUEUE, "Can't put GET_NEXT_N_SKIP into up queue entries");
  queue_[head_ & mask_].downState.request = GET_NEXT_N_SKIP;

  //increment request value by number of tuples asked for
  queue_[head_ & mask_].downState.requestValue += tuples;

   //record that a new get next request has been issued.
  queue_[head_ & mask_].downState.numGetNextsIssued++;

  // schedule the task that handles the GET_NEXT request
  nextSubtask_->schedule();
}

inline void ex_queue::getNextNSkipRequestInit( const Lng32 tuples)
{
  // It better be a down request
  ExQueueAssert(upDown_ == DOWN_QUEUE, "Can't put GET_NEXT_N_SKIP into up queue entries");
  queue_[head_ & mask_].downState.request = GET_NEXT_N_SKIP;

  // set request value by number of tuples asked for
  queue_[head_ & mask_].downState.requestValue = tuples;

  // record that a new get next request has been issued.
  queue_[head_ & mask_].downState.numGetNextsIssued = 1;

  // schedule the task that handles the GET_NEXT request
  nextSubtask_->schedule();
}

inline void ex_queue::getNextNMaybeWaitSkipRequest( const Lng32 tuples)
{
  // It better be a down request
  ExQueueAssert(upDown_ == DOWN_QUEUE, "Can't put GET_NEXT_N_MAYBE_WAIT_SKIP into up queue entries");
  queue_[head_ & mask_].downState.request = GET_NEXT_N_MAYBE_WAIT_SKIP;

  //increment request value by number of tuples asked for
  queue_[head_ & mask_].downState.requestValue += tuples;

   //record that a new get next request has been issued.
  queue_[head_ & mask_].downState.numGetNextsIssued++;

  // schedule the task that handles the GET_NEXT request
  nextSubtask_->schedule();
}

inline void ex_queue::getNextNMaybeWaitSkipRequestInit( const Lng32 tuples)
{
  // It better be a down request
  ExQueueAssert(upDown_ == DOWN_QUEUE, "Can't put GET_NEXT_N_MAYBE_WAIT_SKIP into up queue entries");
  queue_[head_ & mask_].downState.request = GET_NEXT_N_MAYBE_WAIT_SKIP;

  // set request value by number of tuples asked for
  queue_[head_ & mask_].downState.requestValue = tuples;

  // record that a new get next request has been issued.
  queue_[head_ & mask_].downState.numGetNextsIssued = 1;

  // schedule the task that handles the GET_NEXT request
  nextSubtask_->schedule();
}

inline void ex_queue::getNext0MaybeWaitRequest(void)
{
  // It better be a down request
  ExQueueAssert(upDown_ == DOWN_QUEUE, "Can't put GET_NEXT_0_MAYBE_WAIT into up queue entries");
  queue_[head_ & mask_].downState.request = GET_NEXT_0_MAYBE_WAIT;

  //record that a new get next request has been issued.
  queue_[head_ & mask_].downState.numGetNextsIssued++;

  // schedule the task that handles the GET_NEXT request
  nextSubtask_->schedule();
}

inline void ex_queue::getNext0MaybeWaitRequestInit(void)
{
  // It better be a down request
  ExQueueAssert(upDown_ == DOWN_QUEUE, "Can't put GET_NEXT_0_MAYBE_WAIT into up queue entries");
  queue_[head_ & mask_].downState.request = GET_NEXT_0_MAYBE_WAIT;

  //record that a new get next request has been issued.
  queue_[head_ & mask_].downState.numGetNextsIssued = 1;
  queue_[head_ & mask_].downState.requestValue = 0;

  // schedule the task that handles the GET_NEXT request
  nextSubtask_->schedule();
}
//?johannes??

inline ex_queue_entry * ex_queue::getQueueEntry(const queue_index i) const
{
  ExQueueAssert( !isVacant(i),
		 "ex_queue::getQueueEntry getting an empty entry");
  return &queue_[i & mask_]; //(size_-1)];
}

inline ex_queue_entry * ex_queue::getHeadEntry() const
{
  // make sure the head_ is not an empty entry
  ExQueueAssert(head_ != tail_,
		"ex_queue::getHeadEntry() get head on an empty queue");
  return &queue_[head_ & mask_]; //(size_ - 1)];
}

inline queue_index ex_queue::getHeadIndex() const
{
  return head_;
}

Int32 ex_queue::entryExists(queue_index ix) const
{
  return !isVacant(ix);
}

inline void ex_queue::insert()
{
  // Check if the queue is full and no insert is possible
  ExQueueAssert(!isFull(), "ex_queue::insert() Inserting into a full queue_");
  ExQueueAssert(queue_[tail_ & mask_].checkState(upDown_), 
            "ex_queue::insert() Inserting an empty entry into a queue");
#ifdef _DEBUG
  logInsert();
#endif

  tail_++;

#ifdef _DEBUG
  injectErrorOrCancel();
#endif

  // schedule the task that reads from the queue
  insertSubtask_->schedule();
}

inline queue_index ex_queue::getSize() const
{
  return size_;
}

inline ex_cri_desc * ex_queue::getCriDesc()
{
  return criDesc_;
};

inline void atp_struct::copyAtp(const ex_queue_entry *from)
{
  copyAtp(from->getAtp());
}


/////////////////////////////////////////////////////////////////////////////
// Inline procedures -- class up_state

inline Lng32 up_state::getMatchNo(void) const
{
  return matchNo;
};

inline NABoolean up_state::getDoneNoRowsMatchNo(void) const
{
  return (matchNo == GetDoneNoRowsMatchNo);
}

inline Lng32 up_state::setMatchNo(Int64 n)
{
  if (n <= INT_MAX)
    matchNo = (Lng32) n;
  else 
    matchNo = OverflowedMatchNo;
  return matchNo;
}

inline void up_state::setDoneNoRowsMatchNo(void)
{
  matchNo = GetDoneNoRowsMatchNo;
}

#endif


