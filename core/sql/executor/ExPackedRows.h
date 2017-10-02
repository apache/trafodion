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
#ifndef ExUnPackRows_h
#define ExUnPackRows_h


/* -*-C++-*-
******************************************************************************
*
* File:         ExPackedRows.h
* Description:  UnPackRows and PackRows Operator
*               
* Created:      66/19/97
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

// class ExUnPackRowsTdb --------------------------------------------------
// The Task Definition Block for the UnPackRows operator.  This structure is
// produced by the generator and is passed to the executor as part of
// a TDB tree.  This structure contains all the static information 
// necessary to execute the UnPackRows operation.
// 
#include "ComTdbUnPackRows.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ExUnPackRowsTdb;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ExUnPackRowsTdb
// -----------------------------------------------------------------------
class ExUnPackRowsTdb : public ComTdbUnPackRows
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExUnPackRowsTdb()
  {}

  virtual ~ExUnPackRowsTdb()
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
  //    If yes, put them in the ComTdbUnPackRows instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};


// class ExUnPackRowsTcb --------------------------------------------------
// The Task Control Block for the UnPackRows operator.  This structure is
// produced during the build phase as part of the TCB tree.
// This structure contains all the run-time information 
// necessary to execute the UnPackRows operation.
// 
class ExUnPackRowsTcb : public ex_tcb
{
  // The Task Definition Block for the UnPackRows operator.  This struture
  // contains the static information necessary to execute the UnPackRows
  // operator.
  //
  friend class   ExUnPackRowsTdb;

  // The private state for the UnPackRows operator.  This structure contains
  // the information associated with a given request of the UnPackRows TCB.
  //
  friend class   ExUnPackRowsPrivateState;

public:

  // The various states of a request for the UnPackRows work methods.
  //
  enum UnPackChildState
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
  // called during the build phase by ExUnPackRowsTdb::build().
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
  //  ExUnPackRowsTdb &unPackRowsTdb
  //    IN: A reference to the UnPackRows TDB associated with this TCB.
  //
  //  ex_tcb &childTdb
  //    IN: The child TDB of the associated UnPackRows TDB.
  //
  //  ex_globals *glob
  //    IN: Contains references to global executor information,
  //        notably the space object used to allocate objects.
  //
  ExUnPackRowsTcb(const ExUnPackRowsTdb &unPackTdb,
                  const ex_tcb &childTdb,    
                  ex_globals *glob);
          

  // Destructor
  //
  ~ExUnPackRowsTcb();  

  // Free up any run-time resources.
  // For UnPackRows, this frees up the buffer pool.
  // (Does not free up the queues, should it).
  // Called by the destructor.
  //
  void freeResources(); 

  // Register all the UnPackRows subtasks with the scheduler.
  //
  void registerSubtasks();

  // The basic work method for a TCB.  UnPackRows does not
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
  virtual ExWorkProcRetcode workUp();
  
  // Stub to workUp() used by scheduler.
  //
  static ExWorkProcRetcode sWorkUp(ex_tcb *tcb)
  {
    return ((ExUnPackRowsTcb *)tcb)->workUp(); 
  }
  
  // Stub to workDown() used by scheduler.
  // 
  static ExWorkProcRetcode sWorkDown(ex_tcb *tcb)
  {
    return ((ExUnPackRowsTcb *)tcb)->workDown(); 
  }
  
  // Stub to processCancel() used by scheduler.
  //
  static ExWorkProcRetcode sCancel(ex_tcb *tcb)
  {
    return ((ExUnPackRowsTcb *)tcb)->processCancel(); 
  }

  // Return the parent queue pair.
  //
  ex_queue_pair getParentQueue() const { return qParent_; }

  // Return a reference to the UnPackRows TDB associated with this 
  // UnPackRows TCB.
  //
  inline ExUnPackRowsTdb &unPackRowsTdb() const 
  {
    return(ExUnPackRowsTdb &)tdb; 
  }

  inline ex_expr *packingFactor() { return unPackRowsTdb().packingFactor_; }
  
  // Return the UnPackRows expression 
  //
  inline ex_expr *unPackColsExpr() { return unPackRowsTdb().unPackColsExpr_; }

  // UnPackRows has one child.
  //
  virtual Int32 numChildren() const { return 1; }   

  // Return the child of the UnPackRows node by position.
  //
  virtual const ex_tcb * getChild(Int32 pos) const 
  {
    if(pos == 0) return childTcb_;
    return NULL;
  }

protected:
  
  // The child TCB of this UnPackRows node.
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
  
  // Buffer pool used to allocated tupps for the UnPackRows generated
  // columns.
  //
  ExSimpleSQLBuffer *pool_;


  // Atp used to hold tuple from extracting the packing factor (numRows)
  // from the packed row.
  //
  atp_struct * numRowsAtp_;
  tupp numRowsTupp_;
  tupp_descriptor numRowsTuppDesc_;
  Int32 numRows_;

  // Atp used to hold the result of unpacking and hold the tuple used
  // to supply the index value. This value is supplied by the value
  // of indexValue_.
  //
  atp_struct * workAtp_;
  tupp indexValueTupp_;
  tupp_descriptor indexValueTuppDesc_;
  Int32 indexValue_;

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

  // handles an error situation. If the error is nonfatal then we do not go into
  // cancel mode. For both fatal and nonfatal errors a Q_SQLERROR reply is sent, 
  // but a nonfatal error has the rownumber attribute of the Condition set to
  // ComCondition::NONFATAL_ERROR. The markvalue is used for nonfata errors since
  // we have to rewind some of the diags area. Note that the same diags area
  // is used to during UnPackCols expression eval. So for nonfatal errors we need
  // to transfer the conditions to a different diags to prevent the same error
  // from being reported twice.
  void processError(atp_struct *atp, NABoolean isNonFatalError, Lng32 markValue);

  // sends up a Q_REC_SKIPPED reply if UnPackCols expression returns FALSE
  // while we are unpacking rowsets (only if need to count rowNumber). In this case atp
  // should be NULL as there is no DA to send up. This is needed so that the parent node
  // (TF or ONLJ) can assign the correct rowNumber to any future error. This method is also
  // if we detect any rowset row that has raised a Nonfatal error in the CLI. Then atp will
  // be a valid value since we have a diags to send up.
  void processSkippedRow(atp_struct *atp) ;
   
};

class ExUnPackRowsPrivateState : public ex_tcb_private_state
{
  friend class ExUnPackRowsTcb;
  
  Int64 matchCount_; 
  Int32 unPackCount_;

  Int32 numRows_;

  ExUnPackRowsTcb::UnPackChildState childState_;

  Int32 nextCLIErrorRowNum_;

  void init();        

public:

  ExUnPackRowsPrivateState(const ExUnPackRowsTcb * tcb); 

  ex_tcb_private_state * allocate_new(const ex_tcb * tcb);

  ~ExUnPackRowsPrivateState();  
};

// class ExUnPackRowwiseRowsTcb --------------------------------------------------
// The Task Control Block for the UnPackRows operator for rowwise rowsets. 
// 
class ExUnPackRowwiseRowsTcb : public ex_tcb
{
  friend class   ExUnPackRowsTdb;

  // The private state for the UnPackRows operator.  This structure contains
  // the information associated with a given request of the UnPackRows TCB.
  //
  friend class   ExUnPackRowsPrivateState;

public:

  // Constructor
  ExUnPackRowwiseRowsTcb(const ExUnPackRowsTdb &unPackTdb,
			 ex_globals *glob);
          
  ~ExUnPackRowwiseRowsTcb();  

  void freeResources(){}; 

  // Register all the UnPackRows subtasks with the scheduler.
  //
  //  void registerSubtasks();

  // Work method to recieve results from child, process and
  // pass up to parent.
  //
  virtual ExWorkProcRetcode work();
  
  ex_queue_pair getParentQueue() const { return qParent_; }

  inline ExUnPackRowsTdb &uprTdb() const 
  {
    return(ExUnPackRowsTdb &)tdb; 
  }

  virtual Int32 numChildren() const { return 0; }   

  virtual const ex_tcb * getChild(Int32 pos) const 
  {
    return NULL;
  }

private:
  enum Step
  {
    INITIAL_,
    GET_INPUT_VALUES_,
    RETURN_ROW_,
    DONE_,
    ERROR_,
    CANCEL_
  };

  sql_buffer_pool *pool_;

  // The queue pair used to communicate with the parent node.
  // This queue is allocated by this node.
  //
  ex_queue_pair  qParent_;

  // Atp used to hold the result of unpacking and hold the tuple used
  // to supply the index value. This value is supplied by the value
  // of indexValue_.
  //
  atp_struct * workAtp_;
  tupp rwrsInputValuesTupp_;
  tupp_descriptor rwrsInputValuesTuppDesc_;

  Lng32 rwrsNumRows_;
  Lng32 rwrsMaxInputRowlen_;
  char * rwrsBufferAddr_;

  Lng32 currentRowNum_;

  Step step_;
};

// -----------------------------------------------------------------------
// Private state
// -----------------------------------------------------------------------
class ExUnPackRowwiseRowsPrivateState : public ex_tcb_private_state
{
  friend class ExUnPackRowwiseRowsTcb;

  void init();        

public:

  ExUnPackRowwiseRowsPrivateState(const ExUnPackRowwiseRowsTcb * tcb); 

  ex_tcb_private_state * allocate_new(const ex_tcb * tcb);

  ~ExUnPackRowwiseRowsPrivateState();  
};

#endif

