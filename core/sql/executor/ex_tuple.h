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
#ifndef EX_TUPLE_H
#define EX_TUPLE_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ex_tuple.h
 * Description:  Row value constructors for single rows, leaf form is
 *               normally used, non-leaf is used when there was a subquery
 *               in one of the expressions (?)
 *               
 * Created:      4/10/1998
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ComTdbTuple.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ExTupleLeafTdb;
class ExTupleNonLeafTdb;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ExTupleLeafTdb 
// -----------------------------------------------------------------------
class ExTupleLeafTdb : public ComTdbTupleLeaf
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExTupleLeafTdb()
  {}

  virtual ~ExTupleLeafTdb()
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
  //    If yes, put them in the ComTdbTuple instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

// -----------------------------------------------------------------------
// ExTupleNonLeafTdb
// -----------------------------------------------------------------------
class ExTupleNonLeafTdb : public ComTdbTupleNonLeaf
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExTupleNonLeafTdb()
  {}

  virtual ~ExTupleNonLeafTdb()
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
  //    If yes, put them in the ComTdbTuple instead.
  //    If no, they should probably belong to someplace else (like TCB).
  //
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

////////////////////////////////////////
// class ExTupleTcb
////////////////////////////////////////
class ExTupleTcb : public ex_tcb
{
  friend class ComTdbTuple;
  friend class ExTuplePrivateState;

public:
  enum TupleStep {
    TUPLE_EMPTY,
    GET_NEXT_ROW,
    RETURN_TUPLE,
    ALL_DONE,
    CANCEL_REQUEST,
    HANDLE_SQLERROR
  };

 
  ExTupleTcb(const ComTdbTuple & tupleTdb,
	     ex_globals * glob);
 
  ~ExTupleTcb();

  virtual short work();

  void freeResources();

  ex_queue_pair getParentQueue() const
  { 
    return (qparent);
  }

  virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element

  Int32 orderedQueueProtocol() const
  { 
    return ((const ComTdbTuple &)tdb).orderedQueueProtocol();
  };
  
  virtual Int32 numChildren() const { return 0;};
  virtual const ex_tcb* getChild(Int32 pos) const { return NULL;};

protected:
  ex_queue_pair qparent;

  unsigned short tcbFlags_;

  // Atp and buffers to build expressions
  atp_struct * workAtp_;

  ComTdbTuple & tupleTdb() const
  { 
    return (ComTdbTuple &) tdb;
  };

  Queue * tupleExprList() const { return tupleTdb().tupleExprList_; };
};

////////////////////////////////////////////
// class ExTupleLeafTcb
////////////////////////////////////////////
class ExTupleLeafTcb : public ExTupleTcb
{
public:
  ExTupleLeafTcb(const ExTupleLeafTdb & tupleTdb,
		 ex_globals * glob = 0);

  ExWorkProcRetcode work();  
};



////////////////////////////////////////////////
// class ExTupleNonLeafTcb
////////////////////////////////////////////////
class ExTupleNonLeafTcb : public ExTupleTcb
{
public:
  ExTupleNonLeafTcb(const ExTupleNonLeafTdb & tupleTdb,
		    const ex_tcb & tcbChild,
		    ex_globals * glob = 0);

  ExWorkProcRetcode work();  

  virtual Int32 numChildren() const { return 1;};
  virtual const ex_tcb* getChild(Int32 pos) const
  {
    ex_assert((pos >= 0), ""); 
    if (pos == 0)
      return tcbChild_;
    else
      return NULL;
  }
private:
  const ex_tcb * tcbChild_;
  ex_queue_pair  qchild_;

  queue_index nextToSendDown_; // next down queue index to send to server

};

class ExTuplePrivateState : public ex_tcb_private_state
{
  friend class ExTupleTcb;
  friend class ExTupleLeafTcb;
  friend class ExTupleNonLeafTcb;

  Int64  matchCount_; // number of rows returned for this parent row

  ExTupleTcb::TupleStep step_;	// step in processing this parent row
public:	
  ExTuplePrivateState(); //constructor
  ~ExTuplePrivateState();	// destructor
};


#endif
