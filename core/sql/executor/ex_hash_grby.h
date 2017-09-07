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
#ifndef EX_HASH_GRBY_H
#define EX_HASH_GRBY_H

/* -*-C++-*-
******************************************************************************
*
* File:         ex_hash_grby.h
* Description:  Class declarations for ex_hash_grby_tcb and ex_hash_grby_tdb
*               Hash groupby (full and partial hash groupby)
*               
* Created:      
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "hash_table.h"
#include "cluster.h"

// External forward declarations
//
class ExSimpleSQLBuffer;
class ExTrieTable;
class ExBitMapTable;
class ExHashGroupByStats;
class ExBMOStats;

//
// Task Definition Block
//

#include "ComTdbHashGrby.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ex_hash_grby_tdb;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ex_hash_grby_tdb
// -----------------------------------------------------------------------
class ex_hash_grby_tdb : public ComTdbHashGrby
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ex_hash_grby_tdb()
  {}

  virtual ~ex_hash_grby_tdb()
  {}    // LCOV_EXCL_LINE

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
  // corresponding Executor TDB. As a result of this, all Executor TDBs
  // must have absolutely NO data members, but only member functions. So,
  // if you reach here with an intention to add data members to a TDB, ask
  // yourself two questions:
  //
  // 1. Are those data members Compiler-generated?
  //    If yes, put them in the ComTdbHashGrby instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};


/////////////////////////////
// Task control block
/////////////////////////////
class ex_hash_grby_tcb : public ex_tcb {
  friend class   ex_hash_grby_tdb;
  friend class   ex_hash_grby_private_state;
  
  enum HashGrbyState {
    HASH_GRBY_EMPTY,
    HASH_GRBY_READ_CHILD,
    HASH_GRBY_READ_OF_ROW,
    HASH_GRBY_SPILL,
    HASH_GRBY_READ_BUFFER,
    HASH_GRBY_EVALUATE,
    HASH_GRBY_RETURN_ROWS,
    HASH_GRBY_RETURN_BITMUX_ROWS,
    HASH_GRBY_RETURN_PARTIAL_GROUPS,
    HASH_GRBY_DONE,
    HASH_GRBY_ERROR,
    HASH_GRBY_CANCELED
  };

  // the phase of the hash groupby algorithm
  // Change ex_hash_grby_tcb::HashGrbyPhaseStr when you add new value to this enum
  // New phase enum value should be added before PHASE_END
  enum HashGrbyPhase {
    HGB_READ_PHASE,
    HGB_RETURN_PHASE,
    HGB_READ_SPILL_PHASE,
    PHASE_END
  };

  Space * space_;
  CollHeap * heap_;
  MemoryMonitor * memMonitor_;

  const ex_tcb    * childTcb_;
  ex_queue_pair     parentQueue_;
  ex_queue_pair     childQueue_;
   
  // the expressions; copied here for convenience
  ex_expr      * hashExpr_;
  ex_expr      * bitMuxExpr_;
  ex_expr      * bitMuxAggrExpr_;
  ex_expr      * hbMoveInExpr_;
  ex_expr      * ofMoveInExpr_;
  ex_expr      * resMoveInExpr_;
  AggrExpr     * hbAggrExpr_;
  AggrExpr     * ofAggrExpr_;
  AggrExpr     * resAggrExpr_;
  ex_expr      * havingExpr_;
  ex_expr      * moveOutExpr_;
  ex_expr      * hbSearchExpr_;
  ex_expr      * ofSearchExpr_;

  // the atp indexes; copied here for convenience
  short          hbRowAtpIndex_;
  short          ofRowAtpIndex_;
  short          hashValueAtpIndex_;
  short          bitMuxAtpIndex_;
  short          bitMuxCountOffset_;
  short          resultRowAtpIndex_;
  short          returnedAtpIndex_;

  // the following members could also end up in the private state
  Int64             matchCount_;
  HashGrbyState     state_;
  HashGrbyState     oldState_;
  NABoolean         readingChild_;  // in middle of reading child rows
  queue_index       index_;         // index into down queue
  ULng32     bucketCount_;
  Bucket          * buckets_;
  Cluster         * rCluster_;      // the cluster for which we return all
				    // matching rows
  Cluster         * ofClusterList_; // list of spilled clusters
  Cluster         * lastOfCluster_; // last entry in list
  ClusterDB       * clusterDb_;
  sql_buffer_pool * resultPool_;
  NABoolean         hasFreeTupp_;   // for partial group by; indicates wether
                                    // there is an allocated tupp in the
                                    // result buffer or not.

  atp_struct      * workAtp_;
  atp_struct      * tempUpAtp_;
  tupp_descriptor   hashValueTupp_;
  SimpleHashValue   hashValue_;
  HashTableCursor   cursor_;

  ExeErrorCode      rc_;

  // This counter is used to find out how many consecutive misses
  // happen for finding a group during partial groupby.
  // Once a certain threshold is reached(partialGroupbyMissCounter_== 10), 
  // then the rows in the partial groupby bucket is sent to the parent and
  // new set of groups is created.
  Int64             partialGroupbyMissCounter_;

  // Members for implementing the bitMux grouping
  //
  // An ExTrieTable for keeping track of the groups
  //
  //ExTrieTable * bitMuxTable_;

  // An ExBitMapTable for keeping track of the bitMux groups
  //
  ExBitMapTable * bitMuxTable_;

  // A tupp descriptor used to point to the temporary buffer for the extracted
  // grouping data.
  //
  tupp_descriptor bitMuxTupp_;

  // A temporary buffer for the extracted grouping data.
  //
  char * bitMuxBuffer_;

  // The hash buffer aggregate expression to apply to rows that are
  // in the BitMux table. Since the BitMux table computes some aggragates
  // itself, the aggregate expression for rows in the BitMux table do not
  // need to recompute the same aggregates.
  //
  //exp_expr * bitMuxAggrExpr_; -- declared above

  // The offset in the BitMux buffer at which to store the row count.
  //
  //int bitMuxCountOffset_; -- declared above

  ExSubtask *ioEventHandler_;

  IOTimer ioTimer_;

  Int32 numIOChecks_;

  NABoolean haveSpilled_;
  ExBMOStats *bmoStats_;
  ExHashGroupByStats *hashGroupByStats_;

  //CIF buffer defragmentation
  tupp_descriptor * defragTd_;

  void workInitialize();

  void workReadChild();

  Int32 workReadChildBitMux();

  void workReadOverFlowRow();

  void workSpill();

  void workReadBuffer();

  void workEvaluate();

  ULng32 workReturnRows(NABoolean tryToDefrag);

  void workDone();

  void workHandleError(atp_struct* atp = NULL);

  inline void setState(HashGrbyState state) {
    oldState_ = state_;
    state_ = state;
};

  inline NABoolean isReadingFromChild() const { return readingChild_; }

  inline void inReadingFromChild() { readingChild_ = TRUE; }

  inline void doneReadingFromChild() { readingChild_ = FALSE; }

  void returnResultCurrentRow(HashRow * dataPointer = NULL);

  void resetClusterAndReadFromChild(); // Tmobile.

public:
  static const char *HashGrbyPhaseStr[];

  ex_hash_grby_tcb(const ex_hash_grby_tdb & hash_grby_tdb,    
		   const ex_tcb &    childTcb,
		   ex_globals * glob); 
  
  ~ex_hash_grby_tcb();  

  void freeResources();  // free resources

  void registerSubtasks();
  
  short work();  // when scheduled to do work

  inline ex_hash_grby_tdb & hashGrbyTdb() const;

  ex_queue_pair getParentQueue() const { return parentQueue_; }

  virtual const ex_tcb* getChild(Int32 pos) const;

  virtual Int32 numChildren() const { return 1; }
  virtual NABoolean needStatsEntry();
  virtual ExOperStats *doAllocateStatsEntry(CollHeap *heap,
                                                       ComTdb *tdb);

};


/*****************************************************************************
  Description : Return ex_tcb* depending on the position argument.
                  Position 0 means the left most child.
  Comments    :
  History     : Yeogirl Yun                                      8/14/95
                 Initial Revision.
*****************************************************************************/
inline const ex_tcb* ex_hash_grby_tcb::getChild(Int32 pos) const {
  ex_assert((pos >= 0), ""); // LCOV_EXCL_START
  if (pos == 0)
    return childTcb_;
  else
    return NULL; 
}  // LCOV_EXCL_STOP

///////////////////////////////////////////////////////////////////////////
//
//  Inline procedures
//
///////////////////////////////////////////////////////////////////////////

inline ex_hash_grby_tdb & ex_hash_grby_tcb::hashGrbyTdb() const {
  return (ex_hash_grby_tdb &) tdb;
};

//////////////////////////////////////////////////////////////////////////
// The hash groupby operator does not need private state per queue entry
//////////////////////////////////////////////////////////////////////////

#endif



