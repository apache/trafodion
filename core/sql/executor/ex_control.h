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
#ifndef EX_CONTROL_H
#define EX_CONTROL_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ex_control.h
 * Description:  TDB and TCB to do control query statements.
 *               
 *               
 * Created:      5/15/1998
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ComTdbControl.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ExControlTdb;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ExControlTdb
// -----------------------------------------------------------------------
class ExControlTdb : public ComTdbControl
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExControlTdb()
  {}

  virtual ~ExControlTdb()
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
  //    If yes, put them in the ComTdbControl instead.
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
class ExControlTcb : public ex_tcb
{
  friend class ExControlTdb;
  friend class ExControlPrivateState;

public:
  enum Step
  {
    EMPTY_,
    PROCESSING_,
    DONE_,
    CANCELLED_
  };

  ExControlTcb(const ExControlTdb & lock_tdb,
	       ex_globals * glob = 0);

  ~ExControlTcb();

  virtual short work();

  ex_queue_pair getParentQueue() const{return qparent_;};
  inline Int32 orderedQueueProtocol() const{return ((const ExControlTdb &)tdb).orderedQueueProtocol();}

  void display() const{};
  void freeResources();
  
  const ex_tcb* getChild(Int32 /*pos*/) const{return 0;};
  Int32 numChildren() const { return 0; }   

protected:
  ex_queue_pair	qparent_;

  inline ExControlTdb & controlTdb() const{return (ExControlTdb &) tdb;};

};

class ExSetSessionDefaultTcb : public ExControlTcb
{
public:
  ExSetSessionDefaultTcb(const ExControlTdb & control_tdb,
			 ex_globals * glob = 0)
       : ExControlTcb(control_tdb, glob)
  {}

  virtual short work();
};

class ExControlPrivateState : public ex_tcb_private_state
{
  friend class ExControlTcb;
  
public:	
  ExControlPrivateState(const ExControlTcb * tcb); //constructor
  ~ExControlPrivateState();	// destructor
  ex_tcb_private_state * allocate_new(const ex_tcb * tcb);

private:
  ExControlTcb::Step step_;

};

#endif

