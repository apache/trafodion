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
#ifndef EX_HASHJ_H
#define EX_HASHJ_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ex_hashj.h
 * Description:  Hash join
 *               
 *               
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "cluster.h"
#include "ExStats.h"
/////////////////////////////////////////////////////////////////////////////
// Task Definition Block
// for a description of the expressions see GerRelJoin.C
/////////////////////////////////////////////////////////////////////////////
#include "ComTdbHashj.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ex_hashj_tdb;
class ex_hashj_private_state;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;
class ExBMOStats;
class ExHashJoinStats;

// -----------------------------------------------------------------------
// ex_hashj_tdb
// -----------------------------------------------------------------------
class ex_hashj_tdb : public ComTdbHashj
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  NA_EIDPROC ex_hashj_tdb()
  {}

  NA_EIDPROC virtual ~ex_hashj_tdb()
  {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  NA_EIDPROC virtual ex_tcb *build(ex_globals *globals);

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
  // corresponding Executor TDB. As a result of this, all Executor TDBs
  // must have absolutely NO data members, but only member functions. So,
  // if you reach here with an intention to add data members to a TDB, ask
  // yourself two questions:
  //
  // 1. Are those data members Compiler-generated?
  //    If yes, put them in the ComTdbHashj instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

/////////////////////////////////////////////////////
// Task control block
/////////////////////////////////////////////////////
class ex_hashj_tcb : public ex_tcb {
  friend class   ex_hashj_tdb;

public:

  ex_hashj_tcb(const ex_hashj_tdb & hashJoinTdb,  
	       const ex_tcb & leftChildTcb,    // left queue pair
	       const ex_tcb & rightChildTcb,   // right queue pair
	       ex_globals * glob);
        
  ~ex_hashj_tcb();  
  
  void freeResources();
  void registerSubtasks(); 
  ExWorkProcRetcode work();
  
  ex_queue_pair getParentQueue() const
  {
  return (parentQueue_);
  }

  inline ex_hashj_tdb & hashJoinTdb() const;

  virtual const ex_tcb* getChild(Int32 pos) const;
  virtual Int32 numChildren() const { return 2; }   
   

  // the state of the hash join algorithm
  enum HashJoinState {
    HASHJ_EMPTY,
    HASHJ_READ_INNER,
    HASHJ_END_PHASE_1,
    HASHJ_READ_OUTER,
    HASHJ_END_PHASE_2,
    HASHJ_FLUSH_CLUSTER,
    HASHJ_READ_INNER_CLUSTER,
    HASHJ_READ_OUTER_CLUSTER,
    HASHJ_READ_BUFFER,
    HASHJ_PROBE,
    HASHJ_DONE,
    HASHJ_CANCELED,
    HASHJ_ERROR,
    HASHJ_RETURN_RIGHT_ROWS,
    HASHJ_CANCEL_AFTER_INNER
  };

  // the phase of the hash join algorithm
  // Change ex_hashj_tcb::HashJoinPhaseStr when you add new value to this enum
  // New phase enum value should be added before PHASE_END
  enum HashJoinPhase {
    PHASE_1,
    PHASE_2,
    PHASE_3,
    PHASE_END
  };

  static const char *HashJoinPhaseStr[];

  virtual NA_EIDPROC NABoolean needStatsEntry();
  virtual NA_EIDPROC ExOperStats *doAllocateStatsEntry(CollHeap *heap,
                                                       ComTdb *tdb);

protected:
  Space * space_;
  CollHeap * heap_;
  MemoryMonitor *memMonitor_;

  const ex_tcb * leftChildTcb_;
  const ex_tcb * rightChildTcb_;

  ex_queue_pair  parentQueue_;
  ex_queue_pair  leftQueue_;
  ex_queue_pair  rightQueue_;

  // Copy the pointers to the expressions for convenience

  ex_expr * rightHashExpr_;
  ex_expr * rightMoveInExpr_;
  ex_expr * rightMoveOutExpr_;
  ex_expr * rightSearchExpr_;
  ex_expr * leftHashExpr_;
  ex_expr * leftMoveExpr_;
  ex_expr * leftMoveInExpr_;
  ex_expr * leftMoveOutExpr_;
  ex_expr * probeSearchExpr1_;
  ex_expr * probeSearchExpr2_;
  ex_expr * leftJoinExpr_;
  ex_expr * nullInstForLeftJoinExpr_;
  ex_expr * rightJoinExpr_;
  ex_expr * nullInstForRightJoinExpr_;
  ex_expr * beforeJoinPred1_;
  ex_expr * beforeJoinPred2_;
  ex_expr * afterJoinPred1_;
  ex_expr * afterJoinPred2_;
  ex_expr * afterJoinPred3_;
  ex_expr * afterJoinPred4_;
  ex_expr * afterJoinPred5_;
  ex_expr * checkInputPred_;
  ex_expr * moveInputExpr_;
  ex_expr * checkInnerNullExpr_;
  ex_expr * checkOuterNullExpr_;

  // An expression used to compute Min and Max values of one or more
  // of the join values coming from the inner side.  Used when hashj
  // is configured to use the min/max optimization.  If present, the
  // expression is evaluated at the same time as the rightHashExpr.
  ex_expr * minMaxExpr_;

  atp_struct * checkInputAtp_ ;
  tupp prevInputValues_;  // this tupp points to a copy of the previous input

  NABoolean havePrevInput_; // is prevInputValue_ set?
  queue_index prevReqIdx_;  // request idx of previous value

  NABoolean haveAllocatedClusters_;// Have we allocated the clusters (and still
                                   // have their Hash Tables -- for REUSE )

  Int64 totalRightRowsRead_ ; // total # right rows read from the right child

  Cluster * currCluster_; // current cluster being processed (in phase 1)

  NABoolean flushedChainedCluster_; // in case a cluster was flushed after
                                    // being chained, in phase 1

  // buffer pool to keep the input for comparison with the next input (REUSE)
  sql_buffer_pool * inputPool_;

  // For handling efficiently a special case: Anti Semi with no search expr
  NABoolean isAllOrNothing_;
  NABoolean anyRightRowsRead_; // In lieu of a hash-table -- just remember
                               //  if any right row was read

  // number of buckets
  ULng32 bucketCount_;

  NABoolean doNotChainDup_;  // Can HashTable::position ignore duplicates?

  // the buckets
  Bucket * buckets_;

  // the cluster description block
  ClusterDB * clusterDb_;

  // buffer pool for results
  sql_buffer_pool * resultPool_;



  NABoolean outerMatchedInner_;  // did this outer row match any inner row? 
  atp_struct * workAtp_;

  // Used to pass a request to the outer child.  Normally, hashj just
  // passes the request from the parent to both the left and right
  // children.  However, when the hashj is configured to use the
  // min/max optimization, it must add the computed min and max values
  // to the request going to the left child.  In this case, an extra
  // tuple is added to the toLeftChildAtp.  If the min/max
  // optimization is not in effect, then the toLeftChildAtp is the
  // same as the atp comping down from the parent.  Note that the
  // min/max optimization also implies that the request sent to the
  // left child must be delayed until we are finished reading from the
  // right child (finish computing the min and max values).
  atp_struct * toLeftChildAtp_;
  NABoolean hasFreeTupp_;        // do we have a free tupp in the result buffer
  tupp_descriptor hashValueTupp_;
  SimpleHashValue hashValue_;

  ExeErrorCode rc_;

  HashTableCursor cursor_;

  // A local copy of a flag indicating that we need to allocate a
  // tuple for the right row
  //
  const NABoolean isRightOutputNeeded_;

  // Flag: TRUE for all joins, except Left-Join and Anti-Semi-Join
  const NABoolean onlyReturnResultsWhenInnerMatched_;

  // Count # joined rows rejected in this call to workUp.  Is used
  // to make x-product joins return to scheduler in scenarios 
  // where a lot of CPU time is spent to return joined rows at 
  // a very low rate (as can happen if there is a post-join predicate
  // that eliminates almost all rows.  This is to allow the scheduler
  // to enfore CPU limits.
  ULng32 numJoinedRowsRejected_;

  // flag used with hash anti semi join optimization to indicate 
  // whether the inner table is empty or no.
  // it is used to so that we can decide whethre we need to filter out
  // outer NULL values or no.
  NABoolean isInnerEmpty_;

  // Null data tupp for null instantiation.
  // this tupp points to a tuple of all null values the size of the
  // Null instantiated row (hashJoinTdb().instRowForLeftJoinLength_)
  sql_buffer_pool *nullPool_;
  tupp nullData_;


  // work subtasks:

  // work on moving rows down to children
  ExWorkProcRetcode workDown();
  // work on moving result rows up to parent
  ExWorkProcRetcode workUp();
  // process a cancel request
  ExWorkProcRetcode workCancel();

  void propagateCancel(queue_index ix, ex_hashj_private_state &pstate);

  queue_index nextRequest_;    // next request in down queue

  NABoolean allocateClusters();
  NABoolean isSameInputAgain( ex_queue_entry * downParentEntry );

  ExSubtask *ioEventHandler_;
  ExSubtask *workUpTask_;
  ExBMOStats *bmoStats_;
  ExHashJoinStats *hashJoinStats_;

  // static versions
  static ExWorkProcRetcode sWorkDown(ex_tcb *tcb)
                        { return ((ex_hashj_tcb *) tcb)->workDown(); }
  static ExWorkProcRetcode sWorkUp(ex_tcb *tcb)
                          { return ((ex_hashj_tcb *) tcb)->workUp(); }
  static ExWorkProcRetcode sCancel(ex_tcb *tcb)
                    { return ((ex_hashj_tcb *) tcb)->workCancel(); }


  inline Int32   isSemiJoin() const;  // True if we are doing a semi-join
  inline Int32   isLeftJoin() const;  // True if we are doing a left-join
  inline Int32   isRightJoin() const;  // True if we are doing a right-join
  inline Int32   isAntiSemiJoin() const;  // True if its an anti-semi-join
  inline Int32   isReuse() const;  // True if reuse applies
  inline NABoolean leftSideIUD() const;

  // When reuse applies, cancel needs to know if an entry is the
  // first (responsible for building the hash table), in the middle,
  // or the last.  A singleton (not reuesed by subsequent requests) is
  // treated like the last in a series.
  NABoolean isFirst(queue_index ix, ex_hashj_private_state &pstate);
  NABoolean isLast(queue_index ix, ex_hashj_private_state &pstate);

  void releaseResultTupps();
  short insertResult(HashRow * hashRow);
  NABoolean consumeForCancel(ex_queue_pair q);
  short processError(atp_struct *atp);
  short workReadInner();
  void workFlushCluster();
  void workEndPhase1();
  short workReadOuter();
  void workEndPhase2();
  short cleanupLeftRow() ; // utility used only by workProbe()
  void resetClusterForHashLoop(Cluster *iCluster); 
  void prepareForNextPairOfClusters(Cluster *iCluster);
  short workProbe();
  short workReturnRightRows();
  void workReadInnerCluster();
  void workReadOuterCluster();
  void workReadBuffer();
  void workDone();
};

/*****************************************************************************
  Description : Return ex_tcb* depending on the position argument.
                  Position 0 means the left most child.
  Comments    :
  History     : Yeogirl Yun                                      8/14/95
                 Initial Revision.
*****************************************************************************/
inline const ex_tcb * ex_hashj_tcb::getChild(Int32 pos) const {
   ex_assert((pos >= 0), ""); 
   if (pos == 0)
      return leftChildTcb_;
   else if (pos == 1)
      return rightChildTcb_;
   else
      return NULL;
}

//
// return a pair of queue pointers to the parent node. Needed only during
// construction of nodes.
// 

inline ex_hashj_tdb & ex_hashj_tcb::hashJoinTdb() const {
  return (ex_hashj_tdb &) tdb;
};

// check the tdb to see if the join is semi and/or outer
inline Int32  ex_hashj_tcb::isSemiJoin() const {
  return ((ex_hashj_tdb &)tdb).isSemiJoin();
};

inline Int32  ex_hashj_tcb::isLeftJoin() const {
  return ((ex_hashj_tdb &)tdb).isLeftJoin();
};

inline Int32  ex_hashj_tcb::isRightJoin() const {
  return ((ex_hashj_tdb &)tdb).isRightJoin();
};

inline Int32  ex_hashj_tcb::isAntiSemiJoin() const {
  return ((ex_hashj_tdb &)tdb).isAntiSemiJoin();
};

inline Int32  ex_hashj_tcb::isReuse() const {
  return ((ex_hashj_tdb &)tdb).isReuse();
};

inline NABoolean ex_hashj_tcb::leftSideIUD() const {
  return ((ex_hashj_tdb &)tdb).leftSideIUD();
};

///////////////////////////////////////////////
// hash joins private state
//////////////////////////////////////////////
class ex_hashj_private_state : public ex_tcb_private_state
{
  friend class ex_hashj_tcb;
  friend class ExUniqueHashJoinTcb;
  
  ComDiagsArea *accumDiags_;

  ex_hashj_tcb::HashJoinState state_;
  // remember oldState_ to return to after asynchronous I/O is complete
  ex_hashj_tcb::HashJoinState oldState_;
  ex_hashj_tcb::HashJoinPhase phase_;
  NABoolean usingPreviousHT_; // does this request use a HT from a previous request?
  NABoolean delayedLeftRequest_; // Has the request to the left child been
                                 // delayed (Only for allOrNothing )
  NABoolean haveClusters_;    // indicates hash table has been built
  NABoolean readRightRow_ ; // was a row read from the right (for this request)
  Int64 matchCount_;    // # of rows sent to parent
  queue_index reliesOnIdx; // index of request that builds hash table (re)used by this request

public:
  inline void setState(ex_hashj_tcb::HashJoinState state);
  inline ex_hashj_tcb::HashJoinState getState();
  inline void setOldState(ex_hashj_tcb::HashJoinState state);
  inline ex_hashj_tcb::HashJoinState getOldState();
  inline void setPhase(ex_hashj_tcb::HashJoinPhase phase, ExBMOStats *bmoStats);
  inline ex_hashj_tcb::HashJoinPhase getPhase();
  inline void usePreviousHT();
  inline NABoolean usingPreviousHT();
  inline NABoolean delayedLeftRequest() { return delayedLeftRequest_; }
  inline void setDelayedLeftRequest()  { delayedLeftRequest_ = TRUE; }
  inline void resetDelayedLeftRequest()  { delayedLeftRequest_ = FALSE; }
  inline void setHaveClusters(NABoolean needHT);
  inline NABoolean getHaveClusters();

  inline void setReadRightRow() { readRightRow_ = TRUE ; }
  inline NABoolean readRightRow() { return readRightRow_; }

  inline void initState();
  ex_hashj_private_state();
  ex_tcb_private_state * allocate_new(const ex_tcb * tcb);
  ~ex_hashj_private_state();  // destructor
};

inline void ex_hashj_private_state::setState(ex_hashj_tcb::HashJoinState state) {
  oldState_ = state_;
  state_ = state;
}

inline ex_hashj_tcb::HashJoinState ex_hashj_private_state::getState() {
  return (state_);
}

inline void ex_hashj_private_state::setOldState(ex_hashj_tcb::HashJoinState state) {
  oldState_ = state;
}

inline ex_hashj_tcb::HashJoinState ex_hashj_private_state::getOldState() {
  return (oldState_);
}

inline void ex_hashj_private_state::setPhase(ex_hashj_tcb::HashJoinPhase phase, ExBMOStats *bmoStats) {
  phase_ = phase;
  if (bmoStats)
     bmoStats->setBmoPhase(ex_hashj_tcb::HashJoinPhase::PHASE_END-phase);
}

inline ex_hashj_tcb::HashJoinPhase ex_hashj_private_state::getPhase() {
  return (phase_);
}

inline void ex_hashj_private_state::usePreviousHT() {
  usingPreviousHT_ = TRUE ;
}

inline NABoolean ex_hashj_private_state::usingPreviousHT() {
  return (usingPreviousHT_);
}

inline void ex_hashj_private_state::setHaveClusters(NABoolean haveClusters) {
  haveClusters_ = haveClusters;
}

inline NABoolean ex_hashj_private_state::getHaveClusters() {
  return (haveClusters_);
}

inline void ex_hashj_private_state::initState()
{
    accumDiags_ = NULL;
    state_ = ex_hashj_tcb::HASHJ_EMPTY;
    oldState_ = ex_hashj_tcb::HASHJ_EMPTY;
    phase_ = ex_hashj_tcb::PHASE_1;
    matchCount_ = 0;
    usingPreviousHT_ = FALSE;
    delayedLeftRequest_ = FALSE;
    haveClusters_ = FALSE;
    readRightRow_ = FALSE;
}

/////////////////////////////////////////////////////
// Task control block for Unique Hash Join
// Unique Hash Join does not support all the features
// of the more general Hybrid Hash Join, but it can
// have perform much better in many cases.  The Unique
// Hash Join:
//   - does not support overflow
//   - does not support Outer joins (left, right or full)
//   - does not support selection (afterJoinPred) or
//     join (beforeJoinPred) predicates
//   - does not support anti semi join
//   - supports only unique joins (at most one row per probe,
//                                 exception semi joins)
//   - supports Semijoins (even those that are not unique).
//   - joins with equi join predicates (cross products are not supported)
//  
/////////////////////////////////////////////////////
class ExUniqueHashJoinTcb : public ex_hashj_tcb
{
  friend class   ex_hashj_tdb;
  friend class   ex_hashj_tcb;

public:
  
  ExUniqueHashJoinTcb(const ex_hashj_tdb & hashJoinTdb,  
                      const ex_tcb & leftChildTcb,
                      const ex_tcb & rightChildTcb,
                      ex_globals * glob);
        
  ~ExUniqueHashJoinTcb();  
  
  void registerSubtasks(); 
  void freeResources();
  
private:

  // The Unique Hash Join contains a direct reference to the hash table.
  // 
  HashTable *hashTable_;
  
  // The buffer size used to allocate the HashBuffers
  //
  const ULng32 bufferSize_;

  // The size of the row allocated from the HashBuffer and inserted
  // into the hash table
  //
  const ULng32 extRowSize_;

  // The non-extended row size.  This is the size of the row returned
  // from the right hand side of the join.
  //
  const Lng32 rowSize_;

  // The index into the parent up ATP where the right row will be placed.
  //
  const short returnedRightRowAtpIndex_;

  // The number of available rows in the current HashBuffer.
  //
  ULng32 availRows_;

  // The number of rows in the bufferPool (all the HashBuffers).
  //
  ULng32 totalRows_;

  // A Chain of HashBuffers, used to hold the rows of the hash table.
  //
  HashBuffer * bufferPool_;

  // The buffer pools that have been chained.
  //
  HashBuffer * chainedBufferPool_;

  // The Heap used to allocate the hash table and hash buffers.
  //
  NAHeap * bufferHeap_;

public:

  // work subtasks:

  // work on moving result rows up to parent
  ExWorkProcRetcode workUp();

  // Other work subtasks are either not used or come from the
  // ex_hashj_tcb class.

protected:
  // Helper method to allocate the bufferHeap and other necessary structures.
  //
  NABoolean allocateBufferHeap();

  UInt32 computeDefragLength();
private:
  // static versions
  static ExWorkProcRetcode sWorkDown(ex_tcb *tcb)
                        { return ((ExUniqueHashJoinTcb *) tcb)->workDown(); }
  static ExWorkProcRetcode sWorkUp(ex_tcb *tcb)
                          { return ((ExUniqueHashJoinTcb *) tcb)->workUp(); }
  static ExWorkProcRetcode sCancel(ex_tcb *tcb)
                    { return ((ExUniqueHashJoinTcb *) tcb)->workCancel(); }

  // helper work methods.
  //
  short workReadInner(UInt32 defragLength);
  NABoolean workChainInner();
  short workReadOuter();
  short workReturnRow(HashRow *dataPointer,
                      atp_struct *leftRowAtp);

};

#endif
