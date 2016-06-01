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
#ifndef EX_SORT_H
#define EX_SORT_H


/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ex_sort.h
 * Description:  
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

#include "SortUtil.h"
#include "SortUtilCfg.h"
#include "Int64.h"
#include "NABoolean.h"
#include "ComTdbSort.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ExSortTdb;
class ExBMOStats;
class ExSortStats;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ExSortTdb
// -----------------------------------------------------------------------
class ExSortTdb : public ComTdbSort
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  NA_EIDPROC ExSortTdb()
  {}

  NA_EIDPROC virtual ~ExSortTdb()
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
  // corresponding Executor TDB. As a result of this, all Executor TDB's
  // must have absolutely NO data members, but only member functions. So,
  // if you reach here with an intention to add data members to a TDB, ask
  // yourself two questions:
  //
  // 1. Are those data members Compiler-generated?
  //    If yes, put them in the ComTdbSort instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};


//
// Task control block
//
class ExSortTcb : public ex_tcb
{
public:
  enum SortStep {
    SORT_EMPTY,
    RESTART_PARTIAL_SORT,
    SORT_PREP,
    SORT_SEND,
    SORT_RECEIVE,
    SORT_DONE,
    SORT_CANCELED,
    SORT_ERROR,
    SORT_ERROR_ON_RECEIVE,

    // the next states are for SortFromTop only
    SORT_SEND_END,
    SORT_SEND_EOD_TO_CHILD,
    SORT_PROCESS_REPLY_FROM_CHILD,
    SORT_CANCEL_CHILD,
    SORT_DONE_WITH_QND
    };
  
protected:
  friend class   ExSortTdb;
  friend class   ExSortPrivateState;

  const ex_tcb * childTcb_;

  ex_queue_pair  qparent_;
  ex_queue_pair  qchild_;
  
  atp_struct     * workAtp_;
  
  SortType       sortType_;
  SortUtilConfig * sortCfg_;
  SortUtil       * sortUtil_;

  ExSubtask *ioEventHandler_;  
  // this heap is used by sort.
  NAHeap         * sortHeap_;

  // store a sort error in case up queue is full we still have a handle on it
  ComDiagsArea   * sortDiag_;
  NAList<ComDiagsArea *>  *nfDiags_;

  sql_buffer_pool * sortPool_;   // used only for partial sort
  sql_buffer_pool * pool_;     // for normal sorting and and result rows
  sql_buffer_pool *receivePool_;  // can either be sortPool_ or pool_;


  //used to detect change is subset of rows when reading from child.
  //used only in partial sort.
  tupp_descriptor * setCompareTd_;

  tupp_descriptor * defragTd_;

  NABoolean   sortPartiallyComplete_;

 queue_index processedInputs_;

 ExBMOStats *bmoStats_;
 ExSortStats *sortStats_;

  void      createSortDiags();
  NABoolean processSortError(ex_queue_entry *pdown,
			     queue_index parentIndex,
			     queue_index downIndex);

  // Stub to cancel() subtask used by scheduler.
  static ExWorkProcRetcode sCancel(ex_tcb *tcb) 
                         { return ((ExSortTcb *) tcb)->cancel(); }

  short sortSend(ex_queue_entry * centry,
		 ex_queue::up_status child_status,
		 ex_queue_entry * pentry_down,
		 ex_queue_entry * upEntry,
		 NABoolean sortFromTop, //if true,src is parent &tgt is child.
		                        // Otherwise src is child, 
                      		        // tgt is parent
		 SortStep &step,
		 Int64 &matchCount,
		 tupp_descriptor* &allocatedTuppDesc,
		 NABoolean &noOverflow,
		 short &workRC);

  short sortReceive(ex_queue_entry * pentry_down,
		    ex_queue::down_request request,
		    ex_queue_entry * tgtEntry,
		    NABoolean sortFromTop, // src is parent, tgt is child.
                    		           // Otherwise src is child, 
                        		   // tgt is parent
		    queue_index parentIndex,
		    SortStep &step,
		    Int64 &matchCount,
		    tupp_descriptor* &allocatedTuppDesc,
		    NABoolean &noOverflow,
		    short &workRC);

  short done(
       NABoolean sendQND,
       queue_index parentIndex,
       SortStep &step,
       Int64 &matchCount);
  
  short workStatus(short workRC);

  NABoolean resizeCifRecord() const
  {
    return sortCfg_->resizeCifRecord();
  }


  NABoolean considerBufferDefrag() const
  {
    return sortCfg_->considerBufferDefrag();
  }

  Int16 numberOfBytesForRecordSize() const
  {
    return sortCfg_->numberOfBytesForRecordSize();
  }

public:
  // Constructor
  ExSortTcb(const ExSortTdb & sort_tdb,    
	    const ex_tcb &    child_tcb,    // child queue pair
	    ex_globals *glob
	    );
  
  ~ExSortTcb();  
  
  void freeResources();  // free resources
  
  virtual short work();                     // when scheduled to do work
  short workDown();        // pass requests down to child
  short workUp();          // pass results up to parent
  static  short sWorkUp(ex_tcb *tcb)
                           { return ((ExSortTcb *) tcb)->workUp(); }
  static short sWorkDown(ex_tcb *tcb)
                         { return ((ExSortTcb *) tcb)->workDown(); }
  virtual void registerSubtasks();  // register work procedures with scheduler
  short cancel();                   // for the fickle.

  inline ExSortTdb & sortTdb() const { return (ExSortTdb &) tdb; }
 
  ex_queue_pair getParentQueue() const { return qparent_;}

  inline ex_expr * sortKeyExpr() const { return sortTdb().sortKeyExpr_; }
  inline ex_expr * sortRecExpr() const { return sortTdb().sortRecExpr_; }
  inline ULng32 sortBufferSize() const { return sortTdb().bufferSize_;}


  virtual Int32 numChildren() const { return 1; }   
  virtual const ex_tcb* getChild(Int32 /*pos*/) const { return childTcb_; }

  virtual NABoolean needStatsEntry();

  virtual ExOperStats *doAllocateStatsEntry(CollHeap *heap,
					    ComTdb *tdb);

  NA_EIDPROC virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element
};

///////////////////////////////////////////////////////////////////
class ExSortPrivateState : public ex_tcb_private_state
{
  friend class ExSortTcb;
  friend class ExSortFromTopTcb;
  
  ExSortTcb::SortStep step_;

  Int64 matchCount_; // number of rows returned for this parent row
  tupp_descriptor *allocatedTuppDesc_; 
  
  // this is set to FALSE by sort when sortSendEnd() is called,
  // if sorting could not be done in memory and data was written
  // out to disk. It is set to TRUE, if overflow didn't happen.
  NABoolean noOverflow_;

public:
  ExSortPrivateState();        //constructor
  ~ExSortPrivateState();       // destructor
};

class ExSortFromTopTcb : public ExSortTcb
{
  friend class   ExSortTdb;
  friend class   ExSortPrivateState;

public:
  // Constructor
  ExSortFromTopTcb(const ExSortTdb & sort_tdb,    
		   const ex_tcb &    child_tcb,    // child queue pair
		   ex_globals *glob
		   );
  
  ~ExSortFromTopTcb();  

  virtual short work();                     // when scheduled to do work

  virtual void registerSubtasks();  // register work procedures with scheduler

  NA_EIDPROC virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element

private:
  ExSortTcb::SortStep step_;
  Int64 matchCount_; // number of rows returned for this parent row
  tupp_descriptor *allocatedTuppDesc_; 
  
  // this is set to FALSE by sort when sortSendEnd() is called,
  // if sorting could not be done in memory and data was written
  // out to disk. It is set to TRUE, if overflow didn't happen.
  NABoolean noOverflow_;

  // number of input rows sent by parent (NOT including GET_EOD)
  Int64 numParentRows_;

  // number of sorted rows returned by sort
  Int64 numSortedRows_;

  // number of EOD returned by child. One EOD should be returned for each
  // row sent to child unless cancel occurs due to an error.
  Int64 numChildEODs_;

  // set to TRUE after EOD is sent to child. Indicates that all rows have
  // been sent.
  NABoolean eodToChild_;
};

class ExSortFromTopPrivateState : public ex_tcb_private_state
{
  friend class ExSortTcb;
  friend class ExSortFromTopTcb;
  
public:
  ExSortFromTopPrivateState();        //constructor
  ~ExSortFromTopPrivateState();       // destructor
};

#endif
