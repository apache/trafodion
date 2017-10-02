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
#ifndef ExPack_h
#define ExPack_h

/* -*-C++-*-
******************************************************************************
*
* File:         ExPack.h
* Description:  Pack Operator
*               
* Created:      6/16/97
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

#include "ComTdbPackRows.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ExPackRowsTdb;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ExPackRowsTdb
// -----------------------------------------------------------------------
class ExPackRowsTdb : public ComTdbPackRows
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExPackRowsTdb()
  {}

  virtual ~ExPackRowsTdb()
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
  //    If yes, put them in the ComTdbPackRows instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};


class ExPackRowsTcb : public ex_tcb
{
  friend class ExPackRowsTdb;
  friend class ExPackPrivateState;

public:

  // The various states of a request for the Pack work methods.
  enum workState
  {
    // The request has been sent to the child.
    STARTED_, 

    // The request has not yet been sent to the child.
    EMPTY_,   

    // A cancel request has been sent to the child for this request.
    CANCELLED_
  };

  // Constructor
  // Construct a TCB node from the given TDB.  This constructor is
  // called during the build phase by ExPackRowsTdb::build().
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
  ExPackRowsTcb(const ExPackRowsTdb& packTdb,
            const ex_tcb& childTdb,    
            ex_globals* glob);
	  
  // Destructor
  ~ExPackRowsTcb();  

  // Free up any run-time resources.
  void freeResources(); 

  // Register all the pack subtasks with the scheduler.
  void registerSubtasks();

  // The basic work method for a TCB. Pack does not use this method.
  ExWorkProcRetcode work();

  // Work method to pass requests from parent down to child.
  ExWorkProcRetcode workDown();

  // Work method to recieve results from child, process and pass up to parent
  // when the packed record has been formed or there is EOD.
  ExWorkProcRetcode workUp();

  // Work method called by workUp() to return rows when we form a fully packed
  // record or get an EOD before that.
  ExWorkProcRetcode workReturnRow();
  
  // Stub to workUp() used by scheduler.
  static ExWorkProcRetcode sWorkUp(ex_tcb *tcb)
  {
    return ((ExPackRowsTcb *) tcb)->workUp(); 
  } 
  
  // Stub to workDown() used by scheduler.
  static ExWorkProcRetcode sWorkDown(ex_tcb *tcb)
  {
    return ((ExPackRowsTcb *) tcb)->workDown(); 
  }
  
  // Stub to processCancel() used by scheduler.
  static ExWorkProcRetcode sCancel(ex_tcb *tcb)
  {
    return ((ExPackRowsTcb *) tcb)->processCancel(); 
  }

  // Return the parent queue pair.
  ex_queue_pair getParentQueue() const { return qParent_; }

  // Return a reference to the Pack TDB associated with this Pack TCB.
  inline ExPackRowsTdb& packTdb() const
  {
    return (ExPackRowsTdb &) tdb;
  }

  // Return the selection predicate.
  inline ex_expr* predExpr() { return packTdb().predExpr_; }

  // Return the packing expression.
  inline ex_expr* packExpr() { return packTdb().packExpr_; }

  // Pack has one child.
  virtual Int32 numChildren() const { return 1; }   

  // Return the child of the pack node by position.
  virtual const ex_tcb* getChild(Int32 pos) const
  {
    if(pos == 0) return childTcb_;
    return NULL;
  }

protected:
  
  // The child TCB of this Transpose node.
  const ex_tcb* childTcb_;

  // The queue pair used to communicate with the parent node.
  ex_queue_pair qParent_;

  // The queue pair used to communicate with the child node.
  ex_queue_pair qChild_;

  // Next parent down queue entry to process.
  queue_index nextRqst_;
  
  // Buffer pool used to allocated tupps for the packed columns.
  ExSimpleSQLBuffer* pool_;

  // Send next request down to children. (called by workDown())
  void start();

  // Send EOD to parent. (called when child return Q_NO_DATA in queue)
  void stop();

  // Process cancel requests. (called when parent cancel its request)
  ExWorkProcRetcode processCancel(); 
   
};

// -----------------------------------------------------------------------
// Private state
// -----------------------------------------------------------------------
class ExPackPrivateState : public ex_tcb_private_state
{
  friend class ExPackRowsTcb;
  
  tupp packTupp_;
  ExPackRowsTcb::workState childState_;
  Int64 matchCount_; 

  void init();        

public:

  ExPackPrivateState(const ExPackRowsTcb* tcb);

  ~ExPackPrivateState();

  ex_tcb_private_state* allocate_new(const ex_tcb* tcb);

  void printPackTupp();

};

#endif

