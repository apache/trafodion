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
#ifndef ExTranspose_h
#define ExTranspose_h


/* -*-C++-*-
******************************************************************************
*
* File:         ExTranspose.h
* Description:  Transpose Operator
*               
* Created:      4/1/97
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ex_expr.h"

// External forward declarations
//
class ExSimpleSQLBuffer;

// class ExTransposeTdb --------------------------------------------------
// The Task Definition Block for the transpose operator.  This structure is
// produced by the generator and is passed to the executor as part of
// a TDB tree.  This structure contains all the static information 
// necessary to execute the Transpose operation.
// 
#include "ComTdbTranspose.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ExTransposeTdb;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ExTransposeTdb
// -----------------------------------------------------------------------
class ExTransposeTdb : public ComTdbTranspose
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExTransposeTdb()
  {}

  virtual ~ExTransposeTdb()
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
  //    If yes, put them in the ComTdbTranspose instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};



// class ExTransposeTcb --------------------------------------------------
// The Task Control Block for the transpose operator.  This structure is
// produced during the build phase as part of the TCB tree.
// This structure contains all the run-time information 
// necessary to execute the Transpose operation.
// 
class ExTransposeTcb : public ex_tcb
{
  // The Task Definition Block for the transpose operator.  This struture
  // contains the static information necessary to execute the Transpose
  // operator.
  //
  friend class   ExTransposeTdb;

  // The private state for the transpose operator.  This structure contains
  // the information associated with a given request of the transpose TCB.
  //
  friend class   ExTransposePrivateState;

public:

  // The various states of a request for the Transpose work methods.
  //
  enum TransChildState
  {
    // The request has been sent to the child.
    //
    STARTED_, 

    // The request has not yet been sent to the child or
    // this entry has no request.
    //
    EMPTY_,   

    // A cancel request has been sent to the child for this request.
    //
    CANCELLED_
  };

  // Constructor
  // Construct a TCB node from the given TDB.  This constructor is
  // called during the build phase by ExTransposeTdb::build().
  // It:
  //     - allocates the sql buffer pool
  //     - allocates the ATP's for its child's down queue.
  //     - allocates the up and down queues used to communicate 
  //       with the parent.
  //     - allocates the private state associated with each entry of the
  //       parents down queue.
  //     - initializes local state.
  //     - fixes up all expressions.
  //
  // Parameters
  //
  //  ExTransposeTdb &transTdb
  //    IN: A reference to the Transpose TDB associated with this TCB.
  //
  //  ex_tcb &childTdb
  //    IN: The child TDB of the associated Transpose TDB.
  //
  //  ex_globals *glob
  //    IN: Contains references to global executor information,
  //        notably the space object used to allocate objects.
  //
  ExTransposeTcb(const ExTransposeTdb &transTdb,
		 const ex_tcb &childTdb,    
		 ex_globals *glob);
	  

  // Destructor
  //
  ~ExTransposeTcb();  

  // Free up any run-time resources.
  // For transpose, this frees up the buffer pool.
  // (Does not free up the queues, should it).
  // Called by the destructor.
  //
  void freeResources(); 

  // Register all the transpose subtasks with the scheduler.
  //
  void registerSubtasks();

  // The basic work method for a TCB.  Transpose does not
  // use this method, but rather uses three subtasks.
  // - sWorkDown(), sWorkUp() and sCancel().
  //
  ExWorkProcRetcode work();

  // Work method to pass requests from parent down to child.
  //
  ExWorkProcRetcode workDown();

  // Work method to recieve results from child, process and
  // pass up to parent.
  //
  ExWorkProcRetcode workUp();
  
  // Stub to workUp() used by scheduler.
  //
  
  static ExWorkProcRetcode sWorkUp(ex_tcb *tcb)
  {
    return ((ExTransposeTcb *) tcb)->workUp(); 
  }
  
  // Stub to workDown() used by scheduler.
  // 
  static ExWorkProcRetcode sWorkDown(ex_tcb *tcb)
  {
    return ((ExTransposeTcb *) tcb)->workDown(); 
  }
  
  // Stub to processCancel() used by scheduler.
  // 
  static ExWorkProcRetcode sCancel(ex_tcb *tcb)
  {
    return ((ExTransposeTcb *) tcb)->processCancel(); 
  }

  // Return the parent queue pair.
  //
  ex_queue_pair getParentQueue() const { return qParent_; }

  // Return a reference to the Transpose TDB associated with this 
  // Transpose TCB.
  //
  inline ExTransposeTdb &transTdb() const { return (ExTransposeTdb&)tdb; }

  // Return the transpose expression (index by number).
  //
  inline ex_expr * transColExpr(Int32 i) { return transTdb().transColExprs_[i]; }

  // Return the selection Predicate.
  //
  inline ex_expr * afterTransPred() { return transTdb().afterTransPred_; }

  // Transpose has one child.
  //
  virtual Int32 numChildren() const { return 1; }   

  // Return the child of the transpose node by position.
  //
  virtual const ex_tcb * getChild(Int32 pos) const {
    if(pos == 0) return childTcb_;
    return NULL;
  }
  virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element
protected:
  
  // The child TCB of this Transpose node.
  //
  const ex_tcb *childTcb_;

  // The queue pair used to communicate with the parent node.
  // This queue is allocated by this node.
  //
  ex_queue_pair  qParent_;

  // The queue pair used to communicate with the child node.
  // This queue is allocated by the child node.  This data
  // member is initialized by calling getParentQueue() on the child TCB.
  //
  ex_queue_pair  childQueue_;

  // next parent down queue entry to process.
  //
  queue_index processedInputs_;
  
  // Buffer pool used to allocated tupps for the transpose generated
  // columns.
  //
  ExSimpleSQLBuffer *pool_;

  // send next request down to children
  // Called by workDown()
  //
  void start();

  // send EOD to parent
  // Called when child return Q_NO_DATA in queue.
  //
  void stop();

  // Process cancell requests.
  // Called when a cancel request occurs on the parents down queue.
  //
  ExWorkProcRetcode processCancel(); 

  void processError();
   
};

// -----------------------------------------------------------------------
// Private state
// -----------------------------------------------------------------------
class ExTransposePrivateState : public ex_tcb_private_state
{
  friend class ExTransposeTcb;
  
  Int64 matchCount_; 
  Int32 transCount_;

  ExTransposeTcb::TransChildState childState_;

  void init();        

public:

  ExTransposePrivateState(); 

  ex_tcb_private_state * allocate_new(const ex_tcb * tcb);

  ~ExTransposePrivateState();  
};

#endif

