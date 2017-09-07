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
#ifndef EX_ONLJ_H
#define EX_ONLJ_H
/* -*-C++-*-
******************************************************************************
*
* File:         ex_onlj.h
* Description:  Class declarations for ExOnljTcb and ExOnljTdb
*               Ordered Nested Loop Join
*               
* Created:      5/3/94
* Language:     C++
*
*
*
*
******************************************************************************
*/

// This file should only be included by ex_onlj.c

// An onlj is a nested loop join that follows the ordered queue protocol


//
// Task Definition Block
//
#include "ComTdbOnlj.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ExOnljTdb;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ExOnljTdb
// -----------------------------------------------------------------------
class ExOnljTdb : public ComTdbOnlj
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExOnljTdb()
  {}

  virtual ~ExOnljTdb()
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
  //    If yes, put them in the ComTdbOnlj instead.
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
class ExOnljTcb : public ex_tcb
{
  friend class   ExOnljTdb;
  friend class   ExOnljPrivateState;

  const ex_tcb       *tcbLeft_;      // left tcb
  const ex_tcb       *tcbRight_;      // right tcb

  ex_queue_pair  qparent_;
  ex_queue_pair  qleft_;
  ex_queue_pair  qright_;

  queue_index    phaseOne_; // next request to be given from parent to left
  queue_index    phaseTwo_; // next request to be given from left to right

  // Copy the pointer to the expressions for convenience
  // afterPred can only be set if it is an outer join.
  ex_expr       *afterPred_;    // After Join predicate
  // beforePred can only be set if it is an outer or antijoin
  ex_expr       *beforePred_;    // After Join predicate

  // PubSub GET_NEXT_N requests that are canceled when no work has been sent
  // to left child require an extra step in cancel processing: the 
  // work_phase1 method must be scheduled.
  ExSubtask     *exceptionEvent1_;
  // errors coming back from the left child and cleanup of cancelled
  // parent requests are exceptions which are handled by phase 3
  // which sometimes needs to be scheduled explicitly
  ExSubtask     *exceptionEvent3_;

  // Null data tupp for null instantiation.
  // this tupp points to a tuple of all null values the size of the
  // Null instantiated row (onljTdb().ljRecLen_)
  sql_buffer_pool *nullPool_;
  tupp nullData_; 

  ExWorkProcRetcode         work_phase1();
  ExWorkProcRetcode         work_phase2();
  ExWorkProcRetcode         work_phase3();
  virtual ExWorkProcRetcode workCancel();
  NABoolean processNonFatalErrorsInLeftUpQueue(NABoolean &project,
					       NABoolean &consumed);
  
  void cancelParentRequest(ex_queue_entry *x);
  void handleErrorsFromEval(ex_queue_entry *pentry, ex_queue_entry *uentry);

  // static work procedures for scheduler
  static short sWorkPhase1(ex_tcb *tcb)
                          { return ((ExOnljTcb *) tcb)->work_phase1(); }
  static short sWorkPhase2(ex_tcb *tcb)
                          { return ((ExOnljTcb *) tcb)->work_phase2(); }
  static short sWorkPhase3(ex_tcb *tcb) 
                          { return ((ExOnljTcb *) tcb)->work_phase3(); }

  inline Int32   isSemiJoin() const;  // True if we are doing a semi-join
  inline Int32   isLeftJoin() const;  // True if we are doing a left-join
  inline Int32   isAntiJoin() const;  // True if we are doing a anti-join
  inline Int32   vsbbInsertOn() const; // TRUE if right child is a VSBB Insert
  inline Int32   isDrivingMVLogging() const; // True if right child is an isert to an MV IUD log

public:
  // Step in processing the parent row
  // Should really be private but ExOnljPrivateState needs it and making it a
  // a friend doesn't help
  enum nlj_left_step  // would like a short
  {
    NLJ_LEFT_STARTED,    // we have just begun ...
    NLJ_LEFT_EMPTY,      // left did not return any rows
    NLJ_LEFT_NOT_EMPTY,  // Left has returned at least one row; given to right
    NLJ_LEFT_DONE,       // left is done and returned at least one row
    NLJ_LEFT_CANCELLED,  // the request was cancelled before being sent to left
    NLJ_SEND_EOD         // Send a GET_EOD to the right child.
  };

  // Constructor
  ExOnljTcb(const ExOnljTdb &  nlj_tdb,    // 
              const ex_tcb &    left_tcb,    // left queue pair
              const ex_tcb &    right_tcb,   // right queue pair
	      ex_globals *glob
              );
        
  ~ExOnljTcb();  

  inline ExOnljTdb & onljTdb() const;
  
  void        freeResources();  // free resources
  
  void        registerSubtasks();

  short        work();  // when scheduled to do work
  
// return a pair of queue pointers to the parent node. Needed only during
// construction of nodes.
  virtual ex_queue_pair  getParentQueue() const
  {
    return (qparent_);
  }

  virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element

  virtual const ex_tcb* getChild(Int32 pos) const;
  virtual Int32 numChildren() const { return 2; }
};


/*****************************************************************************
  Description : Return ex_tcb* depending on the position argument.
                  Position 0 means the left most child.
  Comments    :
  History     : Yeogirl Yun                                      8/14/95
                 Initial Revision.
*****************************************************************************/
inline const ex_tcb* ExOnljTcb::getChild(Int32 pos) const
{
   ex_assert((pos >= 0), ""); 
   if (pos == 0)
      return tcbLeft_;
   else if (pos == 1)
      return tcbRight_;
   else
      return NULL;
}


class ExOnljPrivateState : public ex_tcb_private_state
{
  friend class ExOnljTcb;
  
  Int64       matchCount_;      // number of rows returned for this parent row
  Int64	      rowCount_;        // number of rows affected by this request. see comments in ex_partn_access.h
  Int64       leftMatches_;     // number of left rows being worked on by right
  Int64       leftOnlyRows_;    // # of left up rows not sent to right
  queue_index leftIndex_;       // index into left down queue
  queue_index startRightIndex_; // index into right down q. (start of rows)
  queue_index endRightIndex_;   // index into right down queue (end of rows)
  short       outerMatched_;    // if true no need to null instatiate
  short       rightRecSkipped_;  // True if the right node has skipped a record. Used only in the case of pub sub using skip conflict access.
  Int64 srcRequestCount_;       // number of q. entries sent to right child from left child (used to set rownumber for rowset error handling)
  Int64 tgtRequestCount_;       // number of Q_NO_DATA seen on right side up queue (used to set rownumber for rowset error handling)
  NABoolean nonFatalErrorSeen_; // to remember that a nonfatal error has been seen
  NABoolean rowAlreadyRaisedNFError_; // to remember that this particular row has already raised a NF error (used to keep track of rowsAfeected correctly )
  // BertBert VV
  // Used for GET_NEXT_N protocol.
  // satisfiedRequestValue = number of rows returned + number of rows we could not satisfy.
  //			   = down-queue-entry.requestValue after a Q_GET_DONE is returned.
  //			   = less than down-queue-entry.requestValue if Q_GET_DONE is not yet returned.
  // down-queue-entry.requestValue = requested number of rows to be returned.
  Lng32 satisfiedRequestValue_;
  // Used for GET_NEXT_N protocol.
  // satisfiedGetNexts = number of Q_GET_DONEs returned
  // down-queue-entry.numGetNexts = number of GET_NEXT_N(_MAYBE_WAIT) received on the down-queue-entry
  Lng32 satisfiedGetNexts_;
  // pushedDownGetNexts = the parent's down-queue.numGetNextsIssued that was moved to 
  //  the child's queue.
  Lng32 pushedDownGetNexts_;
  // TRUE if received a Q_GET_DONE from the left child and we are draining the pipe to the parent.
  // FALSE if the left child is still producing rows to satisfy the current request.
  NABoolean   qGetDoneFromLeft_;
  NABoolean   qNoDataFromLeft_;
  // BertBert ^^
  
  ExOnljTcb::nlj_left_step  leftStep_;

  atp_struct * workAtp_;

  // An accumulator for diags associated with Q_NO_DATA right child queue
  // entries.  This is needed when the onlj is within an ESP with a split
  // top node underneath it.  The split top sends a diags entry with the 
  // Q_NO_DATA that contains the row count.  This must be passed on with
  // the onlj Q_NO_DATA.
  ComDiagsArea *accumDiags_;
  void           init();        // initialize state
public:
  ExOnljPrivateState();
  ~ExOnljPrivateState();  // destructor
};

// get the inline procedures  

// tdb inline procedures
// to determine if it is a semi_join or a left join check the flags_

// check the tdb to see if the join is semi and/or outer

inline Int32  ExOnljTcb::isSemiJoin() const
{
  return ((ExOnljTdb &)tdb).isSemiJoin();
};

inline Int32  ExOnljTcb::isAntiJoin() const
{
  return ((ExOnljTdb &)tdb).isAntiJoin();
};

inline Int32  ExOnljTcb::isLeftJoin() const
{
  return ((ExOnljTdb &)tdb).isLeftJoin();
};

inline Int32  ExOnljTcb::vsbbInsertOn() const
{
  return ((ExOnljTdb &)tdb).vsbbInsertOn();
};

inline Int32 ExOnljTcb::isDrivingMVLogging() const
{
  return ((ExOnljTdb &)tdb).isDrivingMVLogging();
};

inline ExOnljTdb & ExOnljTcb::onljTdb() const
{
  return (ExOnljTdb &)tdb;
}


// end of inline procedures

#endif // EX_ONLJ_H
