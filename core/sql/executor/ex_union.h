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

#ifndef EX_UNION_H
#define EX_UNION_H

/* -*-C++-*-
******************************************************************************
*
* File:         ex_union.h
* Description:  Merge union: combine two data streams and optionally merge
*               them, no duplicate elimination
*               
* Created:      5/3/94
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

// -----------------------------------------------------------------------
// Task Definition Block
// -----------------------------------------------------------------------
#include "ComTdbUnion.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ex_union_tdb;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ex_union_tdb
// -----------------------------------------------------------------------
class ex_union_tdb : public ComTdbUnion
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ex_union_tdb()
  {}

  virtual ~ex_union_tdb()
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
  //    If yes, put them in the ComTdbUnion instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};



// -----------------------------------------------------------------------
// Task control block
// -----------------------------------------------------------------------

class ex_union_tcb : public ex_tcb
{
  friend class   ex_union_tdb;
  friend class   ex_union_private_state;

public:

  enum union_child_state
  {
    STARTED_, 
    EMPTY_,   
    DONE_,    
    CANCELLED_
  };

  // Constructor
  ex_union_tcb(const ex_union_tdb &  union_tdb,    // 
	       const ex_tcb *    left_tcb,    // left queue pair
	       const ex_tcb *    right_tcb,   // right queue pair
	       ex_globals *glob
	       );
        
  ~ex_union_tcb();  

  void        freeResources();  // free resources
  virtual void  registerSubtasks();

  ExWorkProcRetcode work();            // not used for this TCB
  ExWorkProcRetcode workDown();        // pass requests down to child
  ExWorkProcRetcode workUp();          // pass results up to parent


  static  ExWorkProcRetcode sWorkUp(ex_tcb *tcb)
                           { return ((ex_union_tcb *) tcb)->workUp(); }
  static ExWorkProcRetcode sWorkDown(ex_tcb *tcb)
                         { return ((ex_union_tcb *) tcb)->workDown(); }

  static ExWorkProcRetcode sCancel(ex_tcb *tcb)
                    { return ((ex_union_tcb *) tcb)->processCancel(); }
  
inline ex_union_tdb &union_tdb() const
{
        return (ex_union_tdb &) tdb;
};


//
// return a pair of queue pointers to the parent node. Needed only during
// construction of nodes.
//
ex_queue_pair  getParentQueue() const
{
  return (qparent);
}

virtual const ex_tcb* getChild(Int32 pos) const
{
   ex_assert((pos >= 0), ""); 
   if (pos == 0)
      return tcbLeft_;
   else if (pos == 1)
      return tcbRight_;
   else
      return NULL;
}

  inline ex_expr * moveExpr(Int32 i)
  { return (i == 0 ? union_tdb().leftExpr_ : union_tdb().rightExpr_); }

  inline ex_expr * mergeExpr() const { return union_tdb().mergeExpr_; }

  inline ex_expr * condExpr() const { return union_tdb().condExpr_; }

  inline ex_expr * trigExceptExpr() const { return union_tdb().trigExceptExpr_;}

  virtual Int32 numChildren() const { return union_tdb().numChildren(); }
//  virtual const ex_tcb* getChild(int pos) const;
  virtual Int32 hasNoOutputs() const {return FALSE;};
  virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element
protected:

  const ex_tcb       *tcbLeft_;      // left tcb
  const ex_tcb       *tcbRight_;      // right tcb

  ex_queue_pair  qparent;
  ex_queue_pair  childQueues_[2];

  queue_index   processedInputs_;    // next parent down queue entry to process

  virtual void start();		     // send next request down to children
  virtual void stop();		     // send EOD to parent
  virtual ExWorkProcRetcode processCancel(); // check for cancelled request
  virtual void processError(ex_union_private_state &pstate, Int32 &endOfData,
                            atp_struct* atp);

  // A virtual procedure that returns whether a new row can be copied
  // from a child to the parent queue. If the answer is yes, then
  // the method returns which child should be copied, if the answer is
  // no the method returns whether the current request is at end of
  // data (EOD entries in both child queues) or not. The procedure
  // also needs to set pstate.childStates_[i] to DONE_, if appropriate.
  virtual Int32 whichSide(ex_union_private_state &  pstate,
			Int32 &side,
			Int32 &endOfData);
};

// -----------------------------------------------------------------------
// Merge union TCB
// -----------------------------------------------------------------------

class ex_m_union_tcb : public ex_union_tcb
{
public:
  // Constructor
  ex_m_union_tcb(const ex_union_tdb &  union_tdb,    // 
		 const ex_tcb *    left_tcb,    // left queue pair
		 const ex_tcb *    right_tcb,   // right queue pair
		 ex_globals *glob
		 );
        
  virtual Int32 whichSide(ex_union_private_state &  pstate,
			Int32 &side,
			Int32 &endOfData);
};

// -----------------------------------------------------------------------
// Ordered union TCB
// -----------------------------------------------------------------------

class ex_o_union_tcb : public ex_union_tcb
{
public:
  // Constructor
  ex_o_union_tcb(const ex_union_tdb &  union_tdb,    // 
		 const ex_tcb *    left_tcb,    // left queue pair
		 const ex_tcb *    right_tcb,   // right queue pair
		 ex_globals *glob,
		 NABoolean blocked_union,
		 Int32 hasNoOutputs
		 );
        
  virtual Int32 whichSide(ex_union_private_state &  pstate,
			Int32 &side,
			Int32 &endOfData);
  
  ExWorkProcRetcode workDownLeft();        // pass requests down to child

  ExWorkProcRetcode workDownBlockedLeft();        // pass requests down to child
  
  ExWorkProcRetcode workDownRight();        // pass requests down to child

  virtual ExWorkProcRetcode processCancel();     // Cancel request 

  static ExWorkProcRetcode sWorkPhase1(ex_tcb *tcb)
                         { return ((ex_o_union_tcb *) tcb)->workDownLeft(); }

  static ExWorkProcRetcode sWorkBlockedPhase1(ex_tcb *tcb)
                         { return ((ex_o_union_tcb *) tcb)->workDownBlockedLeft(); }

  static ExWorkProcRetcode sWorkPhase2(ex_tcb *tcb)
                         { return ((ex_o_union_tcb *) tcb)->workDownRight(); }

  static ExWorkProcRetcode sCancel(ex_tcb *tcb)
                         { return ((ex_o_union_tcb *) tcb)->processCancel(); }

  virtual void  registerSubtasks();

  virtual Int32 hasNoOutputs() const {return hasNoOutputs_;};

private:
  void startLeftchild();
  void startRightchild();

  // Helper to determine which child's (left or right) parent is farther 
  // from the parent down ex_queue's head_.
  queue_index whichSideParentIndex();

  short rightRequestCnt_;  // a counter for pipelining requests to right child

  // to schedule work method for Right child through a nonQueue event
  ExSubtask *phase2Event_;

  // ++ Triggers -

  // In BLOCKED UNION, there is only one entry served at a time
  // parent down queue is blocked till right child is done.
  //
  // phase3Event is a non-queue event, used to schedule work method for left child 
  // after right child is done, 
  // 
  ExSubtask *phase3Event_;

  NABoolean blockedUnion_;
  Int32 hasNoOutputs_;

  // -- Triggers -

  queue_index  lprocessedInputs_;  // next parent down entry to process 
                                   // for left child
  queue_index  rprocessedInputs_;  // next parent down entry to process
                                   // for right child
};

// -----------------------------------------------------------------------
// Conditional union TCB
// -----------------------------------------------------------------------

class ex_c_union_tcb : public ex_union_tcb
{
public:
  ex_c_union_tcb(const ex_union_tdb &union_tdb,    
		 const ex_tcb *left_tcb, 
		 const ex_tcb *right_tcb,
		 ex_globals *glob);
        
  virtual void registerSubtasks();

  ExWorkProcRetcode condWorkDown();

  static ExWorkProcRetcode sCondWorkDown(ex_tcb *tcb)
                         { return ((ex_c_union_tcb *) tcb)->condWorkDown(); }

//issue a EXE_CS_EOD or a EXE_CS_EOD_ROLLBACK_ERROR type error/warning for
//conditional union
  void processEODErrorOrWarning(NABoolean isWarning) ;

protected:

  virtual void start();		  
  virtual void stop();		

  virtual Int32 whichSide(ex_union_private_state &pstate,
			Int32 &side,
			Int32 &endOfData);

  virtual ExWorkProcRetcode processCancel(); 
  virtual void processError(ex_union_private_state &pstate, Int32 &endOfData,
                            atp_struct* atp);


  ExSubtask *workUpEvent_;         // to schedule workUp method when error has 
                                   // prevented sending request to any child.


};

// -----------------------------------------------------------------------
// Private state
// -----------------------------------------------------------------------

class ex_union_private_state : public ex_tcb_private_state
{
  friend class ex_union_tcb;
  friend class ex_m_union_tcb;
  friend class ex_o_union_tcb;
  friend class ex_c_union_tcb;

  Int64 matchCount_;    // number of rows returned for this parent row

  ex_union_tcb::union_child_state childStates_[2];

  Int32 whichSide_; // 0 or 1, used by whichSide() method ONLY, init to 0, set
 		  // in routine startRightchild() to 1

  Int32 whichChild_; // which side of conditional union to execute.


  void           init();        // initialize state

public:
  
  ex_union_private_state(); //constructor
  ex_tcb_private_state * allocate_new(const ex_tcb * tcb);
  ~ex_union_private_state();  // destructor

  Int32 validChild() const {return whichChild_ >= 0 && whichChild_ <=1; }

};


#endif
