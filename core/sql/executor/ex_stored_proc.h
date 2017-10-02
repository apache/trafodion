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
#ifndef __EX_STORED_PROC_H
#define __EX_STORED_PROC_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ex_stored_proc.h
 * Description:  table-valued, built-in stored procedures that are executed
 *               in a server (currently mxcmp)
 *               
 * Created:      3/12/1997
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "exp_clause_derived.h"

class ExSqlComp;
#include "ComTdbStoredProc.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ExStoredProcTdb;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ExStoredProcTdb
// -----------------------------------------------------------------------
class ExStoredProcTdb : public ComTdbStoredProc
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExStoredProcTdb()
  {}

  virtual ~ExStoredProcTdb()
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
  //    If yes, put them in the ComTdbStoredProc instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};



/////////////////////////////////////
// Task control block
/////////////////////////////////////
class ExStoredProcTcb : public ex_tcb
{
  friend class ExStoredProcTdb;
  friend class ExStoredProcPrivateState;

public:
  enum Step
  {
    BUILD_INPUT_BUFFER_,
    PROCESS_GETNEXT_,
    PROCESS_REQUEST_,
    SEND_INPUT_BUFFER_,
    GET_REPLY_BUFFER_,
    RETURN_ROWS_,
    DONE_,
    CANCELLED_
  };

  // Constructor
  ExStoredProcTcb(const ExStoredProcTdb & ddl_tdb,
		  ex_globals * glob = 0);

  ~ExStoredProcTcb();

  virtual short work();

  ex_queue_pair getParentQueue() const {return qparent_;};

  void freeResources();
  void registerSubtasks(); // add extra event for IPC I/O completion

  Int32 numChildren() const {return 0;};
  const ex_tcb* getChild(Int32 /*pos*/) const{return 0;};

  void tickleScheduler() { ioSubtask_->schedule(); }

private:
  ex_queue_pair	qparent_;

  unsigned short tcbFlags_;

  atp_struct * workAtp_;
  
  Step step_;
  
  SqlBufferBase * returnedBuffer_;
  SqlBufferBase * inputBuffer_;

  // to keep track of the number of input requests sent
  // to arkcmp(SP).
  ULng32 numInputRequests_;

  // pointer to the server used to communicate with ARKCMP to
  // process this SP request.
  ExSqlComp * arkcmp_;

  // subtask to be executed when an I/O completes
  ExSubtask       *ioSubtask_;

  Int64 requestId_;

  inline ExStoredProcTdb & spTdb() const{return (ExStoredProcTdb &) tdb;};

};

class ExStoredProcPrivateState : public ex_tcb_private_state
{
  friend class ExStoredProcTcb;
  friend class ExDescribeTcb;
  
public:	
  ExStoredProcPrivateState(const ExStoredProcTcb * tcb); //constructor
  ~ExStoredProcPrivateState();	// destructor
  ex_tcb_private_state * allocate_new(const ex_tcb * tcb);
protected:
  Int64 matchCount_;
private:
  NABoolean errorHappened_;

};



#endif


