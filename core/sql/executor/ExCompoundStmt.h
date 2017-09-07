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
#ifndef ExCompoundStmt_h
#define ExCompoundStmt_h

/* -*-C++-*-
******************************************************************************
*
* File: 	ExCompoundStmt.h
* Description:  3GL compound statement (CS) operator.
*
* Created:      4/1/98
* Language:     C++
*
*
*
******************************************************************************
*/

#include "ComTdbCompoundStmt.h"

class ex_tcb;

// ExCatpoundStmt implements "catpound statement" semantics. That is, given
//   exec sql begin
//     select c1 into :h1 from t1;
//     select c2 into :h2 from t2;
//   end;
// it produces a single row result set like this
//   c1   c2
//   ---  ---
//   :h1  :h2
// In other words, it restricts any contained selects to be at most single-
// row selects and collapses any single-row result sets into one catenated
// result set. The "catpound statement" result schema is the concatenation 
// of the columns of any contained select statemetns: columns of result set1
// || columns of result set2 || columns of result set3 ... (see ~/generator/
// GenRel3GL.cpp for the "catpound statement" result row layout). 

// A "catpound statement" implementation of the SQL compound statement is 
// a correct implementation provided: 1) the single-row-selects constraint 
// is enforced, and 2) the "catpound" result schema is not exposed to the
// user (eg, it must not show up in sqlci output!).

// True SQL compound statements do not change the logical result of their
// component statements. Thus, in the above example, the true SQL compound
// statement output should look like this
//   c1 
//   ---
//   :h1
// 
//   c2
//   ---
//   :h2
// In other words, the output of executing a compound statement is exactly
// the same as the output of executing the contained statements one after 
// another in (textual) sequential order.

// Therefore, both (the current) "catpound" and (a future true SQL 
// implementation of) compound statement operators must preserve this
// logical sequential execution order. This is an important correctness
// criteria. It means that even a parallel implementation of compound
// statement must preserve logical execution order of the contained SQL
// statements. In terms of the SQLMX pipelined parallel execution engine,
// it may mean that the result sets must be delivered in order in the master 
// executor root node's up queue. An aggressively pipelined implementation 
// of a compound statement operator may execute its left and right children
// in any order but it must deposit the left child's result in the parent's
// up queue first before it can deliver any results from the right child.
// The sequential delivery of result sets to the SQL application program
// is essential to the correct behavior of CLI result set processing calls.

// The prototype implementation of the catpoundstmt operator was patterned
// after the nested loop join (NLJ) operator: ex_onlj.{h,cpp}. But, NLJ is
// a lot more complicated in its bookkeeping because of the need to match
// left and right join columns, the handling of nulls, no-matches, etc.
// As a result, this catpoundstmt operator frequently crashed with orphaned
// queue entries. 

// A catpoundstmt operator should behave more like the ordered union (OU)
// operator that was introduced  to do index maintenance
// where two internally generated SQL statements (a delete and an insert)
// must execute one after another. But, OU also inherits some complicated
// whichSide generic union workUp logic that tries to parallelize (ie,
// non-deterministically interleave) delivery of left and right child 
// results to the parent up-queue. As described above, correct CLI result 
// set processing demands sequential in-order delivery of result sets.
// (It is conceivable that a future aggressively parallel compoundstmt 
// operator may temporarily interleave up-queue processing, but it must
// balance this gain with the penalty of the bookkeeping needed to allow 
// it to eventually deliver result sets in textual sequential order to the
// SQL application). In this initial implementation, we avoid inheriting 
// the complicated OU whichSide workUp logic.

// In addition, the catpoundstmt operator must also deliver and flow any 
// tuple values from the left to the right child just like NLJ but without 
// the complicated matching and null-handling. This left-to-right tuple 
// flow is needed to support catpound semantics (ie, catpound result row
// layout) and the data flow of any assignment statements.

// So, in summary, the catpoundstmt operator executes the left child first;
// flows any tuple values from the left to the right child; starts the
// right child only after the left child is done; delivers any tuple values
// from the right child's up-queue to the parent's up-queue. If right child
// has no data but left child has some, it delivers the left's data result.
// If left has no data but right has some, it delivers the right's data. If
// both left and right have data (OK_MMORE) entries, it delivers the right's
// up-queue entry because the catpoundstmt row layout means it has both left 
// and right tupps catenated together and the left-to-right data flow means
// those left tupps have the left's OK_MMORE data. It also enforces the 
// catpoundstmt at-most-one-row-result constraint. If either left or right 
// child has more than one OK_MMORE entry, it deposits a Q_SQLERROR entry 
// in the parent's up-queue and does a cancel: flushes all OK_MMORE until 
// a Q_NO_DATA. Thus, the 2nd child will not be executed if the 1st child 
// had any error.

// It may seem that this catpoundstmt implementation may degrade in
// performance with bigger catpoundstmts (ones that contain many component
// statements) because of the need to flow data: left-to-right-to-parent.
// But, on second thought, it is probably one of the most (protocol-wise)
// efficient implementation of at-most-one-row-per-select compound 
// statements. In terms of CLI calls, sqlc will translate any (size-n)
// compound statement
//   exec sql begin <stmt1>; <stmt2>; ... <stmtn>; end;
// into just one pair of CLI calls 
//   { ... SQL_EXEC_ExecClose(...) ... SQL_EXEC_FetchClose(...) ... }
// That is, a single Fetch() call fetches all the output host vars of all
// the component statements. 

// Even in the executor, the left-to-right-to-parent data flow is done 
// by filling-in tupps which are pointer (not data) copies. The real data
// copies occur at process (fragment) boundaries. The number of process
// boundaries (and data copies) is probably the same in both catpoundstmt
// and compoundstmt cases.

// -----------------------------------------------------------------------
// ExCatpoundStmtTdb
// -----------------------------------------------------------------------
class ExCatpoundStmtTdb : public ComTdbCompoundStmt
{
public:
  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExCatpoundStmtTdb()
  {}

  virtual ~ExCatpoundStmtTdb()
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
  // corresponding Executor TDB. As a result of this, all Executor TDBs
  // must have absolutely NO data members, but only member functions. So,
  // if you reach here with an intention to add data members to a TDB, ask
  // yourself two questions:
  //
  // 1. Are those data members Compiler-generated?
  //    If yes, put them in the ComTdbCompounStmt instead.
  //    If no, they should probably belong to someplace else (like TCB).
  //
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};


//////////////////////////////////////////////////////////////////////////////
// CatpoundStmt TCB class.
//////////////////////////////////////////////////////////////////////////////
class ExCatpoundStmtTcb : public ex_tcb
{
  friend class ExCatpoundStmtTdb;
  friend class ExCatpoundStmtPrivateState;

public:
  // Standard TCB methods.
  ExCatpoundStmtTcb(const ExCatpoundStmtTdb &tdb,  
                    const ex_tcb &left,   
                    const ex_tcb &right, 
                    ex_globals *glob);
        
  ~ExCatpoundStmtTcb();  

  void freeResources() {}  
  void registerSubtasks();

  virtual const ex_tcb* getChild(Int32 pos) const;
  virtual Int32 numChildren() const { return 2; }   

  ExWorkProcRetcode work();

private:
  // CS TCB specific methods.
  
  ex_queue_pair getParentQueue() const { return qparent_; }

  ExWorkProcRetcode workDownLeft();
  ExWorkProcRetcode workLeft2Right();
  ExWorkProcRetcode workUp();
  ExWorkProcRetcode workCancel();
  
  static ExWorkProcRetcode sWorkDownLeft(ex_tcb *tcb)
  { return ((ExCatpoundStmtTcb *) tcb)->workDownLeft(); }
  static ExWorkProcRetcode sWorkLeft2Right(ex_tcb *tcb)
  { return ((ExCatpoundStmtTcb *) tcb)->workLeft2Right(); }
  static ExWorkProcRetcode sWorkUp(ex_tcb *tcb)
  { return ((ExCatpoundStmtTcb *) tcb)->workUp(); }

  static ExWorkProcRetcode sWorkCancel(ex_tcb *tcb)
  { return ((ExCatpoundStmtTcb *) tcb)->workCancel(); }

  void      startLeftChild();
  void      flowLeft2Right(ex_queue_entry *lentry);
  void      flowParent2Right(queue_index pindex);
  void      passChildReplyUp(ex_queue_entry *centry);

  void      processError(atp_struct *atp, ComDiagsArea *da);
  void      processCardinalityError(ex_queue_entry *centry);
  void      processEODErrorOrWarning(NABoolean isWarning);


  inline ExCatpoundStmtTdb & csTdb() const
    { return (ExCatpoundStmtTdb &) tdb; }

  inline NABoolean expectingRightRows() const
    { return csTdb().expectingRightRows(); }

  inline NABoolean expectingLeftRows() const
    { return csTdb().expectingLeftRows(); }

  inline NABoolean afterUpdate() const
    { return csTdb().afterUpdate(); }

private:
  // CS TCB specific attributes.
  enum CSState {
    CS_EMPTY,          // initial state
    CS_STARTED,        // we began execution
    CS_NOT_EMPTY,      // child has non-EOD row
    CS_ERROR,          // encountered an error
    CS_DONE,           // is done
    CS_CANCELLED       // request cancelled
  };

  const ex_tcb *tcbLeft_;       // TCB for left child.
  const ex_tcb *tcbRight_;      // TCB for right child.

  ex_queue_pair qparent_;       // Queue for parent communication.
  ex_queue_pair qleft_;         // Queue for left child communication.
  ex_queue_pair qright_;        // Queue for right child communication.

  queue_index parent2leftx_;  // index of parent down queue entry being
                              // processed by left child
  queue_index leftupx_;       // left child's up queue index
};

//////////////////////////////////////////////////////////////////////////////
// CatpoundStmtPrivateState class.
//////////////////////////////////////////////////////////////////////////////
class ExCatpoundStmtPrivateState : public ex_tcb_private_state
{
  friend class ExCatpoundStmtTcb;
  
public:
  // Standard private state methods.
  ExCatpoundStmtPrivateState(const ExCatpoundStmtTcb *tcb)
  { init(); }

  ~ExCatpoundStmtPrivateState(){} 

  ex_tcb_private_state * allocate_new(const ex_tcb *tcb);

private:
  void           init();

private:
  // CS Private state attributes.
  Int64 leftrows_;            // Number of left tuples.
  Int64 rightrows_;           // Number of right tuples.

  ExCatpoundStmtTcb::CSState leftstate_;
  ExCatpoundStmtTcb::CSState rightstate_;
};

#endif // ExCompoundStmt_h
