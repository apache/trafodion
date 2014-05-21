/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
#ifndef EX_IAR_H
#define EX_IAR_H 


/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ExIar.h
 * Description:  
 *               
 *               
 * Created:      6/1/2005
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ComTdbInterpretAsRow.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ExIarTdb;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ExIarTdb 
// -----------------------------------------------------------------------
class ExIarTdb: public ComTdbInterpretAsRow
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExIarTdb()
  {}

  virtual ~ExIarTdb()
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
class ExIarTcb : public ex_tcb
{
  friend class   ExIarTdb;
  friend class   ExIarPrivateState;

  public:
  enum IARStep {
     IAR_NOT_STARTED,
     IAR_RETURNING_ROWS,
     IAR_DONE,
     IAR_ERROR
  };

  // Constructor
  ExIarTcb(const ExIarTdb &iarTdb, ex_globals *glob);

  // Destructor
  ~ExIarTcb();

  // free resources
  void freeResources();

  // the real work is done here
  short work();

  // register work procedures with scheduler
  virtual void registerSubtasks();

  short cancel();

  inline ExIarTdb &iarTdb() const { return (ExIarTdb &)tdb; }
  ex_queue_pair getParentQueue() const { return qparent_; }
  virtual Int32 numChildren() const { return 0; }
  virtual const ex_tcb * getChild(Int32 pos) const { return NULL; }

  private:
  ex_queue_pair qparent_;
  atp_struct *workAtp_;
};

///////////////////////////////////////////////////////////////////
class ExIarPrivateState : public ex_tcb_private_state
{
  friend class ExIarTcb;
  
  Int64 matchCount_; // number of rows returned for this parent row
  ExIarTcb::IARStep step_;

  void init(const ExIarTcb *tcb); // initialize state

public:
  ExIarPrivateState(const ExIarTcb * tcb); //constructor
  ex_tcb_private_state * allocate_new(const ex_tcb * tcb);
  ~ExIarPrivateState();       // destructor
};

#endif
