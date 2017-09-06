// **********************************************************************
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
// 
#ifndef EX_MJ_H
#define EX_MJ_H
// -*-C++-*-
// **********************************************************************
// *
// * File:         ex_mj.h
// * Description:  Merge Join
// *               (no repositioning of any child, we read the full data from
// *               each child until reaching EOF. We store duplicate rows
// *               from the inner table in memory, overflowing to temporary
// *               storage when necessary.)
// * Created:      7/10/95
// * Language:     C++
// *
// *
//
// **********************************************************************

// forward declarations
class Queue;

#include "ExOverflow.h"
#include "ExDupSqlBuffer.h"
#include "TupleSpace.h"
#include "exp_expr.h"

/////////////////////////////////////////////
// class ex_mj_tdb: Task Definition Block
/////////////////////////////////////////////
#include "ComTdbMj.h"

using namespace ExOverflow;

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ex_mj_tdb;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ex_mj_tdb
// -----------------------------------------------------------------------
class ex_mj_tdb : public ComTdbMj
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ex_mj_tdb()
  {}

  virtual ~ex_mj_tdb()
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
  //    If yes, put them in the ComTdbMj instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};


/////////////////////////////////////////////////////
// class ex_mj_tcb: Task control block
/////////////////////////////////////////////////////
class ex_mj_tcb : public ex_tcb
{
  friend class   ex_mj_tdb;
  friend class   ex_mj_private_state;

public:
  // Step in processing the parent row
  // Should really be private but ex_mj_private_state needs it and making it a
  // a friend doesn't help
  enum mj_step  // would like a short
  {
    MJ_UNINITIALIZED,
    MJ_EMPTY,
    MJ_GET_LEFT,
    MJ_GET_RIGHT,
    MJ_RETURN_ROW,             // unique merge join only
    MJ_RETURN_ONE_ROW,         // non-unique merge join only
    MJ_SAVE_DUP_RIGHT,         // non-unique merge join only
    MJ_SAVE_DUP_RIGHT_TSPACE,  // non-unique merge join only
    MJ_REWIND_OVERFLOW,        // non-unique merge join only
    MJ_RETURN_SAVED_DUP_ROWS,  // non-unique merge join only
    MJ_RETURN_OVERFLOW_ROWS,   // non-unique merge join only
    MJ_FINISH_LEFT,
    MJ_ERROR,                  // non-unique merge join only
    MJ_CANCEL,
    MJ_DONE,
    MJ_DONE_NEVER_STARTED,
    MJ_NUMBER_OF_STATES        // number of mj_step enumerators
  };

  // Constructor
  ex_mj_tcb(const ex_mj_tdb & mj_tdb,
	    const ex_tcb & left_tcb,
	    const ex_tcb & right_tcb,
	    ex_globals *glob);
        
  ~ex_mj_tcb();  
  
  void freeResources();  // free resources
  
  virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element

  virtual void registerSubtasks();  // register work procedures with scheduler

  short work();  // when scheduled to do work
  
  ex_queue_pair  getParentQueue() const
    {
      return qparent;
    }
  
  void start(ex_mj_private_state& pstate);
  short stop(ex_mj_private_state& pstate);
  short cancel(ex_mj_private_state& pstate);

  virtual const ex_tcb* getChild(Int32 pos) const;
  virtual Int32 numChildren() const { return 2; } 

protected:

  // Opportunistic look ahead states for next left child row.
  enum LookAheadState { LA_LEFT_UNCHECKED,  // check not done
                        LA_LEFT_DUPLICATE,  // next left is duplicate
                        LA_LEFT_DIFFERENT,  // next left is different
                        LA_LEFT_NO_DATA,    // next left is Q_NO_DATA
                        LA_LEFT_ERROR };    // Q_SQLERROR or Q_INVALID

  // Tuple comparison results.  Partial evaluation can return CMP_NOT_EQUAL
  // (see ex_mj_tcb::compareTuples).
  enum Comparison { CMP_ERROR, CMP_LESS, CMP_EQUAL, CMP_GREATER, 
                    CMP_NOT_EQUAL };

  // ATP index for contiguous saved duplicate right row tupp
#if defined(NA_HAS_ANSI_CONST)
  static const Int16 DUP_ATP_INDEX = 2;
#else
  enum { DUP_ATP_INDEX = 2 };
#endif

  const ex_tcb * tcbLeft_;      // left tcb
  const ex_tcb * tcbRight_;     // right tcb

  ex_queue_pair  qparent;
  ex_queue_pair  qleft;
  ex_queue_pair  qright;

  atp_struct * dupAtp_;          // for accessing rows in dupPool_
  tupp * dupTupp_;               // for setting dupAtp_

  // The dupPool_ stores duplicate right child tuples referenced by
  // the rows that merge join returns to its parent.  If this pool has
  // more space than is needed to store these right child tuples, then
  // dupPool_ also stores duplicate right child rows that are
  // candidates for being joined with the current left child row.
  ExDupSqlBuffer * dupPool_;     // duplicate right child row pool

  char * leftEncodedKeyPtr_;
  char * rightEncodedKeyPtr_;

  LookAheadState lookAheadState_;    // opportunistic look ahead state
                                     // for next left child row
  mj_step postIoStep_;               // step to follow I/O completion
  atp_struct * prevRightAtp_;        // right child row for duplicate check
  bool saveFirstDupAtp_;             // if we save the first dup tuple
  ex_expr::exp_return_type savedRetCode_;     
                                     // saved rightCheckDupExpr() result

  // When the fixed-size dupPool_ lacks room to store candidate duplicate 
  // right child rows, these rows are stored in the tspace_ TupleSpace object.
  ExOverflow::TupleSpace * tspace_;  // overflow manager

  atp_struct * workAtp_;

  // Null data tupp for null instantiation.
  // this tupp points to a tuple of all null values the size of the
  // Null instantiated row (mjTdb().ljRecLen_)
  sql_buffer_pool *nullPool_;
  tupp nullData_;

  inline ex_mj_tdb & mjTdb() const
    {
      return (ex_mj_tdb &) tdb;
    }
  
  inline Int32 isSemiJoin() const  // True if we are doing a semi-join
    {
      return mjTdb().isSemiJoin();
    }
  
  inline Int32 isLeftJoin() const  // True if we are doing a left-join
    {
      return mjTdb().isLeftJoin();
    }
  
  inline Int32 isAntiJoin() const  // True if we are doing a Anti-join
    {
      return mjTdb().isAntiJoin();
    }

  inline bool isOverflowEnabled() const
    {
      return mjTdb().isOverflowEnabled();
    }
  
  inline ex_expr * mergeExpr() const
    {
      return mjTdb().mergeExpr_;
    }

  inline ex_expr * compExpr() const
    {
      return mjTdb().compExpr_;
    }

  inline ex_expr * preJoinExpr() const
    {
      return mjTdb().preJoinExpr_;
    }

  inline ex_expr * postJoinExpr() const
    {
      return mjTdb().postJoinExpr_;
    }

  inline ex_expr * leftCheckDupExpr() const
    {
      return mjTdb().leftCheckDupExpr_;
    }

  inline ex_expr * rightCheckDupExpr() const
    {
      return mjTdb().rightCheckDupExpr_;
    }

  inline ex_expr * ljExpr() const
    {
      return mjTdb().ljExpr_;
    }

  inline ex_expr * rightCopyDupExpr() const
    {
      return mjTdb().rightCopyDupExpr_;
    }

  inline ex_expr * leftEncodedKeyExpr() const
    {
      return mjTdb().leftEncodedKeyExpr();
    }

  inline ex_expr * rightEncodedKeyExpr() const
    {
      return mjTdb().rightEncodedKeyExpr();
    }

  Comparison compareTuples(ex_queue_entry * lentry,
                           ex_queue_entry * rentry, 
                           bool doFullComparison = true);

  bool processError(atp_struct * entryAtp = NULL);

  // Reacquire resources (true => success)
  bool reacquireResources(void);

  ex_expr::exp_return_type returnRow(atp_struct * leftAtp,
                                     atp_struct * rightAtp,
                                     ex_mj_private_state & pstate,
                                     bool isUniqueMj = false,
                                     ExOperStats *statsEntry = NULL);

private:
  void checkInit(void);

  void createDiags(Int16 sqlCode = 0);

  // No saved duplicate right rows?
  bool noSavedDups(void);
};

inline const ex_tcb* ex_mj_tcb::getChild(Int32 pos) const
{
   ex_assert((pos >= 0), ""); 
   if (pos == 0)
      return tcbLeft_;
   else if (pos == 1)
      return tcbRight_;
   else
      return NULL;
}

inline
bool 
ex_mj_tcb::noSavedDups(void)
{
  bool zeroDups = !dupPool_->hasDups();
  if (zeroDups && tspace_)
  {
    zeroDups = tspace_->empty();
  }
  return zeroDups;
}

class ex_mj_unique_tcb : public ex_mj_tcb
{
public:
  
  // Constructor
  ex_mj_unique_tcb(const ex_mj_tdb & mj_tdb, 
		   const ex_tcb & left_tcb,  
		   const ex_tcb & right_tcb,
		   ex_globals *glob);

  short work();  // when scheduled to do work
};

///////////////////////////////////////////////
// class ex_mj_private_state
//////////////////////////////////////////////
class ex_mj_private_state : public ex_tcb_private_state
{
  friend class ex_mj_tcb;
  friend class ex_mj_unique_tcb;

  Int64 matchCount_; // number of rows returned for this parent row

  ex_mj_tcb::mj_step step_;

  bool outerMatched_;            // no need to null instantiate?

  static const char* const stateNames[ex_mj_tcb::MJ_NUMBER_OF_STATES];

public:
  ex_mj_private_state();
  ~ex_mj_private_state();

  void init(const ex_mj_tcb *  tcb);        // initialize state

  // Return state name of current state
  const char* currentState(void) const;

private:
  // Return state name of mjStep#
  static const char* stateName(ex_mj_tcb::mj_step mjStep);

};

#endif
