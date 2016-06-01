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
#ifndef EX_SORT_GRBY_H
#define EX_SORT_GRBY_H

/* -*-C++-*-
******************************************************************************
*
* File:         ex_sort_grby.h
* Description:  Class declarations for ex_sort_grby_tcb and ex_sort_grby_tdb
*               The sort groupby operator assumes that its input comes grouped
*               already, therefore it needs no internal table
* Created:      
* Language:     C++
*
*
*
*
******************************************************************************
*/

// External forward declarations
//
class ExSimpleSQLBuffer;

// This file should only be included by ex_sort_grby.c

//
// Task Definition Block
//
#include "ComTdbSortGrby.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ex_sort_grby_tdb;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ex_sort_grby_tdb
// -----------------------------------------------------------------------
class ex_sort_grby_tdb : public ComTdbSortGrby
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  NA_EIDPROC ex_sort_grby_tdb()
  {}

  NA_EIDPROC virtual ~ex_sort_grby_tdb()
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
  //    If yes, put them in the ComTdbSortGrby instead.
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
class ex_sort_grby_tcb : public ex_tcb
{
  friend class   ex_sort_grby_tdb;
  friend class   ex_sort_grby_private_state;

  const ex_tcb * childTcb_;

  ex_queue_pair  qparent_;
  ex_queue_pair  qchild_;

  ExSimpleSQLBuffer *pool_;

  queue_index    processedInputs_;

  atp_struct *workAtp_;

public:
  // Constructor
NA_EIDPROC
  ex_sort_grby_tcb(const ex_sort_grby_tdb & sort_grby_tdb,    
		   const ex_tcb &    child_tcb , // child queue pair
		   ex_globals * glob
		   );
  
NA_EIDPROC
  ~ex_sort_grby_tcb();  
  
  enum sort_grby_step {
    SORT_GRBY_EMPTY,
    SORT_GRBY_NEW_GROUP,
    SORT_GRBY_FIRST_GROUP_ROW,
    SORT_GRBY_STARTED,
    SORT_GRBY_FINALIZE,
    SORT_GRBY_FINALIZE_CANCEL,
    SORT_GRBY_FINALIZE_EOF,
    SORT_GRBY_DONE,
    SORT_GRBY_CANCELLED,
    SORT_GRBY_LOCAL_ERROR,
    SORT_GRBY_CHILD_ERROR,
    SORT_GRBY_NEVER_STARTED
    };
  
NA_EIDPROC
  void freeResources();  // free resources
  
NA_EIDPROC
  short work();  // when scheduled to do work

NA_EIDPROC
  inline ex_sort_grby_tdb & sort_grby_tdb() const;

// return a pair of queue pointers to the parent node. Needed only during
// construction of nodes.
NA_EIDPROC
  ex_queue_pair getParentQueue() const
{
  return (qparent_);
}


NA_EIDPROC
  virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element

NA_EIDPROC
  inline ex_expr * aggrExpr() const { return sort_grby_tdb().aggrExpr_; };
NA_EIDPROC
  inline ex_expr * grbyExpr() const { return sort_grby_tdb().grbyExpr_; };
NA_EIDPROC
  inline ex_expr * moveExpr() const { return sort_grby_tdb().moveExpr_; };
NA_EIDPROC
  inline ex_expr * havingExpr() const { return sort_grby_tdb().havingExpr_; };

NA_EIDPROC
  inline Lng32 recLen() {return sort_grby_tdb().recLen_;};

  NA_EIDPROC virtual Int32 numChildren() const { return 1; }   
  NA_EIDPROC virtual const ex_tcb* getChild(Int32 pos) const;
};

inline const ex_tcb* ex_sort_grby_tcb::getChild(Int32 pos) const
{
   ex_assert((pos >= 0), ""); 
   if (pos == 0)
      return childTcb_;
   else
      return NULL;
}


///////////////////////////////////////////////////////////////////////////
//
//  Inline procedures
//
///////////////////////////////////////////////////////////////////////////

inline ex_sort_grby_tdb & ex_sort_grby_tcb::sort_grby_tdb() const
{
        return (ex_sort_grby_tdb &) tdb;
};

///////////////////////////////////////////////////////////////////
class ex_sort_grby_private_state : public ex_tcb_private_state
{
  friend class ex_sort_grby_tcb;
  
public:

  NA_EIDPROC ex_sort_grby_private_state();
  NA_EIDPROC ~ex_sort_grby_private_state();

private:

  ex_sort_grby_tcb::sort_grby_step step_;

  queue_index index_;        // index into down queue
  Int64 matchCount_;         // number of rows returned for this parent row
  NABoolean     oneRowAggr_;
};


#endif

