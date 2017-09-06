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
#ifndef EX_SPLIT_TOP_H
#define EX_SPLIT_TOP_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ex_split_top.h
 * Description:  Split Top executor node (the client part of a split node)
 *               
 * Created:      12/6/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "NABitVector.h"

////////////////////////////////////////////////////////////////////////////
// Contents of this file
////////////////////////////////////////////////////////////////////////////

class ex_split_top_tdb;
class ex_split_top_tcb;
class ex_split_top_private_state;

////////////////////////////////////////////////////////////////////////////
// Forward references
////////////////////////////////////////////////////////////////////////////

class ComDiagsArea;
class ExPartInputDataDesc;

////////////////////////////////////////////////////////////////////////////
// Task Definition Block for split top node
////////////////////////////////////////////////////////////////////////////

#include "ComTdbSplitTop.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ex_split_top_tdb;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ex_split_top_tdb
// -----------------------------------------------------------------------
class ex_split_top_tdb : public ComTdbSplitTop
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ex_split_top_tdb()
  {}

  virtual ~ex_split_top_tdb()
  {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

private:
  // ---------------------------------------------------------------------
  // !!!!!!! IMPORTANT -- NO DATA MEMBERS ALLOWED IN EXECUTOR TDB !!!!!!!!
  // *********************************************************************
  // The Executor TDB's are only used for the sole purpose of providing a
  // way to supplement the Compiler TDB's (in comexe) with methods whose
  // implementation depends on Executor objects. This is done so as to
  // decouple the Compiler from linking in Executor objects unnecessarily.
  //
  // When a Compiler generated TDB arrives at the Executor, the same data
  // image is "cast" as an Executor TDB after unpacking. Therefore, it is
  // a requirement that a Compiler TDB has the same object layout as its
  // corresponding Executor TDB. As a result of this, all Executor TDB's
  // must have absolutely NO data members, but only member functions. So,
  // if you reach here with an intention to add data members to a TDB, ask
  // yourself two questions:
  //
  // 1. Are those data members Compiler-generated?
  //    If yes, put them in the ComTdbSplitTop instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};


////////////////////////////////////////////////////////////////////////////
// Task control block for split top node
////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------
// private helper struct for ex_split_top_tcb to remember child states
// -----------------------------------------------------------------------

// value for a child that is not associated to any partition number
static const Int32 SPLIT_TOP_UNASSOCIATED = -1;

struct SplitTopChildState
{
  // Information on whether the child is currently active on behalf
  // of a request from the parent (from the parent's point of view)
  // and what the last parent queue index was that was sent down
  // to the child.
  Lng32           numActiveRequests_;
  queue_index    highWaterMark_;
  
  // The next fields are used when the split top node acts as a PAPA
  // node (PArent of Partition Access nodes). PA nodes work best if
  // their requests deal with only one partition number. This ensures
  // that multiple requests can be sent as a single message down to DP2.
  // Using multiple PAs for a large request of a partitioned table
  // allows a variable degree of parallelism. PA nodes do not provide
  // parallel access to multiple partitions, this is done by the PAPA node.
  Lng32           associatedPartNum_;
};

// used for fast indexing to child tcb from a given partition
struct SplitTopPartNums
{
  // The index to a PA in child array which is currently associated
  // to the partition. Valid only if static partition affinity is on.
  CollIndex      childTcbIndex_;
};

struct SplitTopReadyChild
{
  CollIndex                  index;    // child TCB index
  queue_index                prev;     // prev readyChildren_ index
  queue_index                next;     // next readyChildren_ index
  queue_index                entryCnt; // number of ready entries
};

class ex_split_top_tcb : public ex_tcb
{
  // friend class ex_split_top_tdb;
  friend class ex_split_top_private_state;
  friend class ex_split_top_private_state_ext;

public:

  enum workState
  {
    INITIAL,			// entry is untouched by a work() procedure
    PART_NUMS_CALCULATED,	// now we know where to send requests,
				// some requests may have been sent and
				// we may be returning unmerged data
    ALL_SENT_DOWN,		// all down requests sent
    MERGING,                    // merging data from children
    END_OF_DATA,		// all children done, parent needs end-of-data
    CANCELLING,			// cancelling while children may be active
    CANCELLED,			// cancelled, no child is active

    // BertBert VVV
				// NOTE: PART_NUMS_CALCULATED is used to process
				//  GET_ALL and GET_N commands
    PROCESS_GET_NEXT_N,		// used for non-streaming destructive select
				//  processes GET_NEXT_N command
    PROCESS_GET_NEXT_N_MAYBE_WAIT,
				// used for streaming destructive select
				//  processes GET_NEXT_N_MAYBE_WAIT command
    PROCESS_GET_NEXT_N_MAYBE_WAIT_ONE_PARTITION,
				// used for streaming destructive select where only
				//  one partition is accessed.
				//  processes GET_NEXT_N_MAYBE_WAIT command 
    // BertBert ^^^
    ERROR_BEFORE_SEND
  };

  // Constructor
  ex_split_top_tcb(const ex_split_top_tdb & splitTopTdb,
		   ExExeStmtGlobals * glob);
        
  ~ex_split_top_tcb();  

  inline const ex_split_top_tdb & splitTopTdb() const
                                   { return ( const ex_split_top_tdb &)tdb; }
  
  void freeResources();  // free resources
  void registerSubtasks();

  // 2 methods to add child TCB trees dynamically
  void registerChildQueueSubtask(Int32 c);
  void addChild(Int32 c, NABoolean atWorktime, ExOperStats* statsEntry);
  
  short work();  // when scheduled to do work
 
  ex_queue_pair getParentQueue() const;

  virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element

  // access predicates in tdb
  inline ex_expr * childInputPartFunction() const
                           { return splitTopTdb().childInputPartFunction_; }
  inline ex_expr * mergeKeyExpr() const
                                     { return splitTopTdb().mergeKeyExpr_; }
  inline Lng32 mergeKeyAtpIndex() const
                                 { return splitTopTdb().mergeKeyAtpIndex_; }
  inline Lng32 mergeKeyLength() const
                                   { return splitTopTdb().mergeKeyLength_; }
  inline NABoolean isPapaNode() const
                          { return (splitTopTdb().paPartNoAtpIndex_ >= 0); }
 
  virtual Int32 numChildren() const;
  virtual const ex_tcb* getChild(Int32 pos) const;

  virtual ExOperStats *doAllocateStatsEntry(CollHeap *heap,
                                                       ComTdb *tdb);

private:

  ex_queue_pair    qParent_;

  // serves as a temp. child Atp. The sidTuple is attached to this
  // and sent to Aux. PA for sequence generation.
  atp_struct       *tempChildAtp_;

  atp_struct       *workAtp_;
  tupp_descriptor  partNumTupp_;      // target of part # expression
  Lng32             calculatedPartNum_;// target buffer for part. function expr
  SqlBuffer       *inputDataTupps_;  // partition input data sent to children
  SqlBuffer       *paPartNumTupps_;  // part # data sent to PA children
  SqlBuffer       *mergeKeyTupps_;   // encoded keys for children for merge
  ExSubtask        *workDownTask_;    // to schedule workDown method
  ExSubtask        *workUpTask_;      // to schedule workUp method

  ARRAY(ex_tcb *)  childTcbs_;            // ptrs to child TCBs
  ARRAY(ex_queue*) childTcbsParentUpQ_;   // ptrs to child TCBs parent up queue.
  ARRAY(ex_queue*) childTcbsParentDownQ_; // ptrs to child TCBs parent dn queue.
  SplitTopReadyChild* readyChildren_;     // list of data-ready children TCBs

  SplitTopChildState *childStates_;   // states of child PAs
  Lng32             maxNumChildren_;   // upper limit for # of child TCBs
  Lng32             firstPartNum_;     // upper limit for # of child TCBs
  Lng32             numChildren_;      // # of child TCBs used so far
  SplitTopPartNums *childParts_;      // partNum to PA mapping
  LIST(CollIndex)  mergeSequence_;    // ordered list of child #s
  NABitVector      unmergedChildren_; // who is missing from mergeSequence_

  ExOperStats      **statsArray_;     // to keep stats Entrys for all partitions (PAPA)

  // Which partitions (for PAPA node) an INSERT request was sent to.
  // Used to send the EOD (End Of Data) request to those partitions
  // only.
  NABitVector                 partNumsReqSent_;

  // This field is used to accumulate all requests that were previously
  // sent in multiple executions. Set when GET_EOD_NO_ST_COMMIT is seen.
  // Values in this field are then used at GET_EOD to send EOD to all
  // partitions.
  NABitVector                 accumPartNumsReqSent_;

  queue_index      processedInputs_;  // first q entry not yet seen by work()

  // To prevent a race condition in repartitioned plans (some descendent tcb
  // is a multi-parent split_bottom), we only send down one request until
  // all replies are received.  

  NABoolean serializeRequests_;

  enum SplitTopWorkState
    {
    READY_TO_REQUEST,
    WAIT_FOR_ALL_REPLIES,
    } tcbState_;

  // Keep stream timeout - dynamic if set, else the static value
  Lng32 streamTimeout_ ;

  // shared pool used by children (PAs)
  sql_buffer_pool *sharedPool_;

  // private methods

  // work subtasks:

  // work on moving rows down to children
  ExWorkProcRetcode workDown();
  // work on moving result rows up to parent
  ExWorkProcRetcode workUp();
  // process cancel requests
  ExWorkProcRetcode workCancel();

  // static versions
  static ExWorkProcRetcode sWorkDown(ex_tcb *tcb)
                        { return ((ex_split_top_tcb *) tcb)->workDown(); }
  static ExWorkProcRetcode sWorkUp(ex_tcb *tcb)
                          { return ((ex_split_top_tcb *) tcb)->workUp(); }

  // cancel all child activity for a given request from the parent
  void cancelChildRequests(queue_index parentIndex); // for a given parent req.

  // find the child that is associated to partition number partNo or
  // try to associate a child with that partition number
  CollIndex getAssociatedChildIndex(Lng32 partNo);
  void resetAssociatedChildIndex(Lng32 partNo);

  // Generate the next value for the sequence 
  short generateSequenceNextValue(atp_struct * parentAtp);

  // set the partition range for the current parent queue entry
  // (for PAPA node only)
  Lng32 getDP2PartitionsToDo(atp_struct *parentAtp,
			    ex_split_top_private_state *pstate);

  // find the next child that has data ready to copy to the parent
  CollIndex findNextReadyChild(ex_split_top_private_state *pstate);

  inline void addPartNumReqSent(CollIndex p)    { partNumsReqSent_ += p; }
  inline void clearPartNumsReqSent()     { partNumsReqSent_.clearFast(); }
  inline CollIndex getFirstPartNumReqSent() const
                                      { return getNextPartNumReqSent(0); }
  CollIndex getNextPartNumReqSent(CollIndex prev) const;
  void allocateStatsEntry(Int32 c, ex_tcb *childTcb);

  void clearAccumPartNumsReqSent() { accumPartNumsReqSent_.clearFast(); }
  void accumulatePartNumsReqSent();
  void useAccumulatedPartNumsReqSent();

  // Table N-M way repartition mapping, valid only for hash2 partitions:
  //
  //                     ------------------------------------
  //  numOfTopParts (m)  |    |    |    |    |    |    |    |
  //                     ------------------------------------
  //  feeds                 |  /  \ /  \  /  /  \  /  \  /
  //                     ------------------------------------
  //  numOfBotParts (n)  |      |      |      |      |      |
  //                     ------------------------------------
  //
  // For EPS i where i in [0, m), the number of source ESPs can be found
  // by this function:
  inline Lng32 numOfSourceESPs(Lng32 numOfTopPs, Lng32 numOfBottomPs,
                              Lng32 myIndex)
  {
    ex_assert(myIndex < numOfTopPs,
              "Invalid N-M repartition mapping at top!");
    return ((myIndex * numOfBottomPs + numOfBottomPs - 1) / numOfTopPs
           - (myIndex * numOfBottomPs) / numOfTopPs + 1);
  }

  // To find the index of my first child ESP:
  inline Lng32 myFirstSourceESP(Lng32 numOfTopPs, Lng32 numOfBottomPs,
                              Lng32 myIndex)
  { return ((myIndex * numOfBottomPs) / numOfTopPs); }
};

class ex_split_top_private_state : public ex_tcb_private_state
{
  friend class ex_split_top_tcb;
  
public:

  ex_split_top_private_state();
  ~ex_split_top_private_state();

  void init();
  void setHeap(CollHeap *heap);

  inline void addPartNumToDo(CollIndex p)          
  { 
    partNumsToDo_.addElementFast(p); 
  }
  inline void addPartNumToDoRange(CollIndex lo, CollIndex hi)
  { 
    for (CollIndex i = lo; i <= hi; i++) 
      partNumsToDo_.addElementFast(i); 
  }
  inline void removePartNumToDo(CollIndex p)       { partNumsToDo_ -= p; }
  inline void clearPartNumsToDo()           { partNumsToDo_.clearFast(); }
  inline NABoolean partNumsToDoIsEmpty() const
                                       { return partNumsToDo_.isEmpty(); }

  // iterate through partitions to do (end is indicated by NULL_COLL_INDEX)
  inline CollIndex getFirstPartNumToDo() const
                                         { return getNextPartNumToDo(0); }
  CollIndex getNextPartNumToDo(CollIndex prev) const;

  inline void addActiveChild(CollIndex c)        
                                    { activeChildren_.addElementFast(c); }
  inline void removeActiveChild(CollIndex c)     { activeChildren_ -= c; }
  inline CollIndex getPrevActiveChild(CollIndex prev)
         { return activeChildren_.prevUsed(prev); }
  inline CollIndex getLastActiveChild()
         { return getPrevActiveChild(activeChildren_.getLastStaleBit()); }
  inline CollIndex getLastStaleChild()
         { return activeChildren_.getLastStaleBit(); }
  inline CollIndex getFirstActiveChild() { return getNextActiveChild(0); }
  CollIndex getNextActiveChild(CollIndex prev);

  inline CollIndex getNumActiveChildren() const
                                     { return activeChildren_.entries(); }
  inline const NABitVector &getAllActiveChildren() const
                                               { return activeChildren_; }
  inline CollIndex getCurrActiveChild()       { return currActiveChild_; }
  inline void setCurrActiveChild(CollIndex c)    { currActiveChild_ = c; }

  inline ex_split_top_tcb::workState getState() const
           { return (ex_split_top_tcb::workState)state_; }
  void setState(ex_split_top_tcb::workState s);

  void addDiagsArea(ComDiagsArea * diagsArea);
  inline ComDiagsArea * getDiagsArea() const        { return diagsArea_; }
  inline ComDiagsArea * detachDiagsArea()
        { ComDiagsArea *res = diagsArea_; diagsArea_ = NULL; return res; }

  enum flagvals {
    ACTIVE_PART_NUM_CMD_SENT = 0x0001,
    MAINTAIN_GET_NEXT_COUNTERS = 0x0002,
    REC_SKIPPED = 0x0004
  };

  NABoolean activePartNumCmdSent() const { 
    return (splitTopPStateFlags_ & ACTIVE_PART_NUM_CMD_SENT) ? TRUE : FALSE;
  }
  void setActivePartNumCmdSent() { splitTopPStateFlags_ |= ACTIVE_PART_NUM_CMD_SENT ;}
  void clearActivePartNumCmdSent() { splitTopPStateFlags_ &= ~ACTIVE_PART_NUM_CMD_SENT ;}

  NABoolean maintainGetNextCounters() const {
    return (splitTopPStateFlags_ & MAINTAIN_GET_NEXT_COUNTERS) ? TRUE : FALSE;
  }
  void setMaintainGetNextCounters() { splitTopPStateFlags_ |= MAINTAIN_GET_NEXT_COUNTERS ;}
  void clearMaintainGetNextCounters() { splitTopPStateFlags_ &= ~MAINTAIN_GET_NEXT_COUNTERS ;}

  NABoolean recSkipped() const {
    return (splitTopPStateFlags_ & REC_SKIPPED) ? TRUE : FALSE;
  }
  void setRecSkipped() { splitTopPStateFlags_ |= REC_SKIPPED ;}
  void clearRecSkipped() { splitTopPStateFlags_ &= ~REC_SKIPPED ;}

private:

  // state of processing this request
  UInt16                      state_;

  UInt16                      splitTopPStateFlags_;

  // which DP2 partition numbers (for PAPA node) still have to be done
  NABitVector                 partNumsToDo_;

  // which child TCBs are processing requests on behalf of this request
  NABitVector                 activeChildren_;

  // child which is currently returning rows to the parent
  CollIndex                   currActiveChild_;

  // how many rows sent back to parent so far
  Int64                       matchCount_;

  // errors that have accumulated before we can reply to this request
  ComDiagsArea                *diagsArea_;

  // how many OK_MMORE or Q_SQLERRORS sent back to parent so far. Used for Get_N requests and row counting in NJ node.
  Int64                       matchCountForGetN_;
};


// This class represents the extended version of the SplitTop PState.
// The extended version contains state required for GET_NEXT_N
// processing.  The non-extended version of the PState is much smaller
// and can save a lot of memory when very large queues are used.
// The SplitTop PState class was split into two version to reduce the amount of
// memory required, especially when using large queues for Split Top.
class ex_split_top_private_state_ext : public ex_split_top_private_state
{
  friend class ex_split_top_tcb;

public:

  ex_split_top_private_state_ext();
  ~ex_split_top_private_state_ext();

  void init();
  void setHeap(CollHeap *heap);

private:
  // BertBert VVV
  // Variables used to process GET_NEXT_N commands.
  // GET_NEXT_N is used to execute a non-streaming destructive select.
  // Partitions must be processed one at a time, activePartNum is the
  //  partition that we are retrieving deleted/updated rows from via the GET_NEXT_N
  //  protocol.  activePartNumCmdSent remembers if we are waiting for a
  //  response from the active partition.
  CollIndex                   activePartNum_;

  // Variables used to process GET_NEXT_N_MAYBE_WAIT commands.
  // The two bitvectors combined define a state machine:
  //	(A) init/no rows available (commendSent_==0, rowsAvailable_==0)
  //	(B) waiting for row availability (commendSent_==1, rowsAvailable_==0)
  //	(C) rows available (commendSent_==0, rowsAvailable_==1)
  //	(D) retrieving rows (commendSent_==1, rowsAvailable_==1)
  //  Transitions:
  //	(A) -> (B): send a GET_NEXT_0_MAYBE_WAIT
  //	(B) -> (C): receive a Q_GET_DONE
  //	(C) -> (D): send a GET_NEXT_N
  //	(D) -> (C): receive rows plus Q_GET_DONE
  //	(D) -> (A): receive a Q_GET_DONE
  NABitVector                 commandSent_;
  NABitVector		      rowsAvailable_;
  Lng32			      rowsBeforeQGetDone_; // this assumes one active partition at a time.

  // Used for GET_NEXT_N protocol.
  // satisfiedRequestValue = number of rows returned + number of rows we could not satisfy.
  //			   = down-queue-entry.requestValue after a Q_GET_DONE is returned.
  //			   = less than down-queue-entry.requestValue if Q_GET_DONE is not yet returned.
  // down-queue-entry.requestValue = requested number of rows to be returned.
  Lng32			      satisfiedRequestValue_;
  // Used for GET_NEXT_N protocol.
  // satisfiedGetNexts = number of Q_GET_DONEs returned
  // down-queue-entry.numGetNexts = number of GET_NEXT_N(_MAYBE_WAIT) received on the down-queue-entry
  Lng32			      satisfiedGetNexts_;
  // The counters above are signed 32 bit, so we need a guard to prevent them 
  // from being incremented in the non-pubsub code path.

  // microseconds time when we last timed out the stream (streamTimeout_ is the .01 seconds to timeout).
  Int64			      time_of_stream_get_next_usec_;

  // BertBert ^^^

};

#endif /* EX_SPLIT_TOP_H */

