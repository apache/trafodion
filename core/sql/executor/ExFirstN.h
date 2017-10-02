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
#ifndef EX_FIRSTN_H
#define EX_FIRSTN_H


/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ex_firstn.h
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

#include "Int64.h"
#include "NABoolean.h"
#include "ComTdbFirstN.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ExFirstNTdb;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ExFirstNTdb
// -----------------------------------------------------------------------
class ExFirstNTdb : public ComTdbFirstN
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExFirstNTdb()
  {}

  virtual ~ExFirstNTdb()
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
  //    If yes, put them in the ComTdbFirstn instead.
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
class ExFirstNTcb : public ex_tcb
{
  friend class   ExFirstNTdb;
  friend class   ExFirstNPrivateState;

  enum FirstNStep {
    INITIAL_,
    PROCESS_FIRSTN_,
    PROCESS_LASTN_,
    DONE_,
    CANCEL_,
    ERROR_
    };

  const ex_tcb * childTcb_;

  ex_queue_pair  qparent_;
  ex_queue_pair  qchild_;

  FirstNStep step_;

  Int64 requestedLastNRows_;
  Int64 returnedLastNRows_;

  atp_struct     * workAtp_;
  Lng32 firstNParamVal_;   // first N computed from parameter
  Lng32 effectiveFirstN_;  // effective first n (constant or param)
  Lng32 returnedSoFar_;    // number of rows returned so far

  // Stub to cancel() subtask used by scheduler. 
  static ExWorkProcRetcode sCancel(ex_tcb *tcb) 
  { return ((ExFirstNTcb *) tcb)->cancel(); }
  
public:
  // Constructor
  ExFirstNTcb(const ExFirstNTdb & firstn_tdb,    
	      const ex_tcb &    child_tcb,    // child queue pair
	      ex_globals *glob
	      );
  
  ~ExFirstNTcb();  
  
  short moveChildDataToParent();

  void freeResources();  // free resources
  
  short work();                     // when scheduled to do work
  virtual void registerSubtasks();  // register work procedures with scheduler
  short cancel();                   // for the fickle.

  inline ExFirstNTdb & firstnTdb() const { return (ExFirstNTdb &) tdb; }

 
  ex_queue_pair getParentQueue() const { return qparent_;}


  virtual Int32 numChildren() const { return 1; }   
  virtual const ex_tcb* getChild(Int32 /*pos*/) const { return childTcb_; }

}; 

///////////////////////////////////////////////////////////////////
class ExFirstNPrivateState : public ex_tcb_private_state
{
  friend class ExFirstNTcb;
  
public:
  ExFirstNPrivateState(const ExFirstNTcb * tcb); //constructor
  ex_tcb_private_state * allocate_new(const ex_tcb * tcb);
  ~ExFirstNPrivateState();       // destructor
};

#endif
