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
#ifndef EXVPJOIN_H
#define EXVPJOIN_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ExVPJoin.h
 * Description:  Header file for VP Join operator that merges vertical partitions
 *               
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ComTdb.h"
#include "ex_tcb.h"
#include "ex_queue.h"
#include "ExpCriDesc.h"
#include "ex_expr.h"
#include "ex_globals.h"

// 
// OVERVIEW
//
// ExVPJoinTdb and associated classes implement an operator that
// merges vertical partitions.  This operator is similar to a merge
// join, except (1) a merge join has exactly two input streams, while
// ExVPJoin may have more than two, and (2) the children of an
// ExVPJoin produce the same number of rows in exactly the same key
// sequence, while this is not necessarily the case with the more
// general merge join.
// 

class ExVPJoinTdb;
class ExVPJoinTcb;
class ExVPJoinTcbPrivateState;

#include "ComTdbVPJoin.h"

// Exclude this code from coverage analysis since this feature is
// obsolete and not used.
// LCOV_EXCL_START

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ExVPJoinTdb;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ExVPJoinTdb
// -----------------------------------------------------------------------
class ExVPJoinTdb : public ComTdbVPJoin
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExVPJoinTdb()
  {}

  virtual ~ExVPJoinTdb()
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
  //    If yes, put them in the ComTdbVPJoin instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};



class ExVPJoinTcb: public ex_tcb
{
  friend class ExVPJoinTdb;
  friend class ExVPJoinPrivateState;

public:
  
  ExVPJoinTcb(const ExVPJoinTdb & vpjTdb, // TDB from which to build TCB
			 const ex_tcb ** childTcbs,  // child TCBs
			 ex_globals *glob            // globals
			 );
        
  ~ExVPJoinTcb();

  void freeResources();
  ExWorkProcRetcode work();
  void registerSubtasks();

  const ex_tcb* getChild(Int32 pos) const;
  ex_queue_pair  getParentQueue() const;
  virtual Int32 numChildren() const;
    


private:

  // Work methods.
  //
  ExWorkProcRetcode workDown();
  ExWorkProcRetcode workUp();
  ExWorkProcRetcode workCancel();

  // Static work procedures for scheduler.
  //
  static short sWorkDown(ex_tcb *tcb);
  //                          { return ((ExVPJoinTcb *) tcb)->workDown(); }
  static short sWorkUp(ex_tcb *tcb);
  //                          { return ((ExVPJoinTcb *) tcb)->workUp(); }
  static short sWorkCancel(ex_tcb *tcb);
  //                          { return ((ExVPJoinTcb *) tcb)->workCancel(); }

  inline ExVPJoinTdb & vpJoinTdb() const;
  
  // Cancel request from parent.  pReq points to (parent down)
  // queue entry containing the request, and pIx identifies the 
  // queue index of this entry.
  void cancelParentRequest(ex_queue_entry *pReq, queue_index pIx);

  // Event by which we can tell the scheduler to call workUp().  This
  // is sometimes necessary to handle cancelled requests correctly.
  ExSubtask *exceptionEvent_;

  // Data members.
  //

  const ex_tcb ** childTcbs_;   // array of pointers to child task control blocks

  ex_queue_pair qParent_; // parent queue
  ex_queue_pair *qChild_; // array of pointers to child queues

  queue_index nextReqIx_; // index of next request, in parent down queue,
                          // to send to child nodes
  
  Int32 numChildren_;       // number of children

  inline ex_expr * filterPred();
};

class ExVPJoinPrivateState : public ex_tcb_private_state
{
  friend class ExVPJoinTcb;

public:
  ExVPJoinPrivateState(const ExVPJoinTcb * tcb);
  ex_tcb_private_state * allocate_new(const ex_tcb * tcb);
  ~ExVPJoinPrivateState();

private:
  inline void init();

  Int64 matchCount_; // number of rows returned for 
                     // associated request

  Int32 started_; // has associated request been "started"
                // (i.e., passed down to children)
};

void ExVPJoinPrivateState::init()
{
  matchCount_ = 0;
  started_ = 0;
}

inline ExVPJoinTdb & ExVPJoinTcb::vpJoinTdb() const
{
  return (ExVPJoinTdb &)tdb;
}

inline ex_expr * ExVPJoinTcb::filterPred() 
{ 
  return vpJoinTdb().filterExpr_; 
}
 
// LCOV_EXCL_STOP

#endif

