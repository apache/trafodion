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
#ifndef EX_TUPLE_FLOW_H
#define EX_TUPLE_FLOW_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ex_tuple_flow.h
 * Description:  Similar to nested join, but this operator can handle
 *               insert VSBB and will send a GET_EOD request down to the
 *               right child after receiving and EOD from the left child.
 * Created:      2/25/1997
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ComTdbTupleFlow.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ExTupleFlowTdb;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ExTupleFlowTdb
// -----------------------------------------------------------------------
class ExTupleFlowTdb : public ComTdbTupleFlow
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExTupleFlowTdb()
  {}

  virtual ~ExTupleFlowTdb()
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
  //    If yes, put them in the ComTdbTupleFlow instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};


class ExTupleFlowTcb : public ex_tcb
{
  friend class   ExTupleFlowTdb;
  friend class   ExTupleFlowPrivateState;

  const ex_tcb       *tcbSrc_;      // source(left)  tcb
  const ex_tcb       *tcbTgt_;      // target(right) tcb

  ex_queue_pair  qParent_;
  ex_queue_pair  qSrc_;
  ex_queue_pair  qTgt_;

public:
  // Step in processing the parent row
  enum TupleFlowStep 
  {
    EMPTY_, MOVE_SRC_TO_TGT_, MOVE_EOD_TO_TGT_,
    PROCESS_TGT_, HANDLE_ERROR_, CANCELLED_, DONE_
  };

  // Constructor
  ExTupleFlowTcb(const ExTupleFlowTdb & tuple_flow_tdb, 
		 const ex_tcb &    src_tcb,
		 const ex_tcb &    tgt_tcb,
		 ex_globals *glob
		 );
        
  ~ExTupleFlowTcb();  

  ExTupleFlowTdb & tflowTdb() const {return (ExTupleFlowTdb &)tdb;};
  
  void        freeResources();  // free resources
  
  short        work();  // when scheduled to do work
  
  ex_queue_pair  getParentQueue() const {return qParent_;};

  // for GUI
  Int32 numChildren() const { return 2; }   
  const ex_tcb* getChild(Int32 pos) const
  {
    ex_assert((pos >= 0), ""); 
    if (pos == 0)
      return tcbSrc_;
    else if (pos == 1)
      return tcbTgt_;
    else
      return NULL;
  }
};


class ExTupleFlowPrivateState : public ex_tcb_private_state
{
  friend class ExTupleFlowTcb;
  
  Int64 matchCount_;      // number of rows returned for this parent row
  Lng32 tgtRequests_;
  NABoolean srcEOD_;
  NABoolean parentEOD_;
  NABoolean tgtRowsSent_;
  Lng32 noOfUnPackedRows_;
  // next two counters used to set rownumber for rowset error handling
                                // The next two should be converted to Int64, 
                                // except then we'd also need to 
                                // convert ComDiagsArea::setAllRowNumber 
                                // to take an Int64...
  Int64 srcRequestCount_;        // number of q. entries sent to right child from left child 
  NABoolean nonFatalErrorSeen_;      // to remember that a nonfatal error has been seen
  Int64 startRightIndex_ ;      // index to remember the earliest parent request for which we have
                                // not seen a reply yet. Used in the CANCELLED_ state. 
 
  ExTupleFlowTcb::TupleFlowStep  step_;

  atp_struct * workAtp_;

  void           init();        // initialize state

public:
  ExTupleFlowPrivateState(const ExTupleFlowTcb * tcb); //constructor
  ex_tcb_private_state * allocate_new(const ex_tcb * tcb);
  ~ExTupleFlowPrivateState();  // destructor
};








#endif

