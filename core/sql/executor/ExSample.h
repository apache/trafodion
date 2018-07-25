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
#ifndef ExSample_h
#define ExSample_h

/* -*-C++-*-
******************************************************************************
*
* File:         ExSample.h
* Description:  Class declarations for ExSample, a sample scan operator.
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

// Task Definition Block
//
#include "ComTdbSample.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ExSampleTdb;
class ExSampleTcb;
class ExSamplePrivateState;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;
class ex_tdb;

// -----------------------------------------------------------------------
// ExSampleTdb
// -----------------------------------------------------------------------
class ExSampleTdb : public ComTdbSample
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExSampleTdb()
  {};

  virtual ~ExSampleTdb()
  {};

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
class ExSampleTcb : public ex_tcb
{
  friend class   ExSampleTdb;
  friend class   ExSamplePrivateState;
  
public:
  enum RequestState {
    ExSamp_EMPTY,
    ExSamp_PREWORK,
    ExSamp_WORKING,
    ExSamp_RETURNINGROWS,
    ExSamp_CANCELLED,
    ExSamp_ERROR,
    ExSamp_DONE
  };
  
  ExSampleTcb(const ExSampleTdb & sample_tdb,    
			 const ex_tcb &    child_tcb,
			 ex_globals * glob
			 );
  ~ExSampleTcb();  
  
  void freeResources();
  short work();
  
  inline ExSampleTdb & myTdb() const;
  ex_queue_pair getParentQueue() const
  {
  return (qparent_);
  }

  inline ex_expr * initExpr() const;
  inline ex_expr * balanceExpr() const;
  inline Int32 returnFactorOffset() const;
  inline ex_expr * postPred() const;
  
  virtual Int32 numChildren() const;
  virtual const ex_tcb* getChild(Int32 pos) const;

private:
  const ex_tcb * childTcb_;
  
  ex_queue_pair  qparent_;
  ex_queue_pair  qchild_;
  
  queue_index    processedInputs_;
}; // class ExSampleTcb

// ExSampleTdb inline function definitions
//
inline ex_expr * ExSampleTcb::initExpr() const 
{ 
  return myTdb().initExpr_; 
};

inline ex_expr * ExSampleTcb::balanceExpr() const 
{ 
  return myTdb().balanceExpr_; 
};

inline Int32 ExSampleTcb::returnFactorOffset() const
{ 
  return myTdb().returnFactorOffset_; 
};

inline ex_expr * ExSampleTcb::postPred() const 
{ 
  return myTdb().postPred_; 
};
  
inline Int32 ExSampleTcb::numChildren() const 
{ 
  return 1; 
}   

inline const ex_tcb* ExSampleTcb::getChild(Int32 pos) const
{
   ex_assert((pos >= 0), ""); 
   if (pos == 0)
      return childTcb_;
   else
      return NULL;
}

inline ExSampleTdb & ExSampleTcb::myTdb() const
{
        return (ExSampleTdb &) tdb;
};


// class ExSamplePrivateState
//
class ExSamplePrivateState : public ex_tcb_private_state
{
  friend class ExSampleTcb;

public:
  ExSamplePrivateState(const ExSampleTcb * tcb);
  ex_tcb_private_state * allocate_new(const ex_tcb * tcb);
  ~ExSamplePrivateState();

private:
  ExSampleTcb::RequestState step_;
  queue_index index_;  
  Int64 matchCount_;
  ULng32 rowsToReturn_;
  atp_struct * workAtp_;
};


#endif








