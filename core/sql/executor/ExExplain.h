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
#ifndef EX_EXPLAIN_H
#define EX_EXPLAIN_H


/* -*-C++-*-
******************************************************************************
*
* File:         ExExplain.h
* Description:  Class declarations for ExExplainTcb
*               
*               
* Created:      4/26/96
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "exp_attrs.h"

class ExplainDesc;
class ExplainTuple;

//
// Task Definition Block for Explain Function:
//
// Notable contents:
// 
// -  scanPred_ a scan predicate to be applied to each tuple in the
//    explain tree.
//
// -  paramsExpr - a contiguous move expression to be applied to the input
//    which will populate a tuple with the parameters.

#include "ComTdbExplain.h"
// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ExExplainTdb;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ExExplainTdb
// -----------------------------------------------------------------------
class ExExplainTdb : public ComTdbExplain
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExExplainTdb()
  {}

  virtual ~ExExplainTdb()
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
  //    If yes, put them in the ComTdbExplain instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};


//
// Task control block from Explain Function
//
// Notable contents:
//

class ReposTextChunksInfo
{
 public:
  void init()
  {
    numChunks_ = 0;
    totalLen_ = 0;
  }

  Int32 numChunks_;
  Int32 totalLen_; // total length of all chunks
};

// this structure is stored in repository and preceeds the actual explain data.
// It contains details about explain storage and chunks info, if explain plan
// was chunked into multiple rows and stored in metric_text_table.
class ExplainReposInfo
{
 public:
  void init()
    {
      filler_ = 0;
      rtci_.init();
    }

  Int64 filler_;
  ReposTextChunksInfo rtci_;
};

class ExExplainTcb : public ex_tcb
{
public:

  // The return code for the traversal method (see traverseTree() below)
  // This enum has to be public, in order for the c89 compiler to be
  // able to compile ex_explain.C
  enum traverseReturnCode {
    EXPL_TRAV_DONE,		// traverseTree() returned because the
				// traversal was completed.
    
    EXPL_TRAV_QUEUEFULL,	// traverseTree() returned because its
				// parent up queue was full and it could
				// not proceed at this time.  When called
				// again it will pick up where it left off.
    EXPL_TRAV_NOBUFFERS,	// traverseTree() returned because it
				// could not get a buffer and it could
				// not proceed at this time.  When called
				// again it will pick up where it left off.
    EXPL_TRAV_ERROR
    };

  // Constructor called during build phase (ExExplainTdb::build()).
  ExExplainTcb(const ExExplainTdb & explainTdb, ex_globals *glob);
        
  // Default destructor
  ~ExExplainTcb();  

  // Free resources (Don't know exactly what this should do)
  void  freeResources();

  // The work procedure for ExExplainTcb.
  // For details see ex_explain.C
  short work();

  // The queue pair used to communicate with the parent TCB
  ex_queue_pair  getParentQueue() const;

  // A virtual function used by the GUI.  Will always return 0 for
  // ExExplainTcb
  virtual Int32 numChildren() const;
  virtual const ex_tcb *getChild(Int32 pos) const;

  // Method used to allocate and initialize the the paramsTuple.
  // Also two buffers are allocated to hold the values of the
  // parameters to the explain function.
  // The paramsTuple will contain the values of the parameters
  // to the explain function (module name and statement pattern)
  // The paramsTuple will be populated by evaluating the paramsExpr
  void initParamsTuple(Int32 tupleLength,
		       ex_cri_desc *criDescParams,
		       Lng32 lengthModName,
		       Lng32 lengthStmtPattern);
  
  void setQid(char *qid, Lng32 len);
  void setReposQid(char *reposQid, Lng32 len);

  void setExplainAddr(char *addr, Lng32 len);
  void setExplainAddr(Int64 addr);
  void setExplainStmt(char *stmt, Lng32 len);
  void setExplainPlan(char *plan, Lng32 len);

  RtsExplainFrag *sendToSsmp();

  static short getExplainData(
                              ex_root_tdb * rootTdb,
                              char * explain_ptr,
                              Int32 explain_buf_len,
                              Int32 * ret_explain_len,
                              ComDiagsArea *diagsArea,
                              CollHeap * heap);

  static short storeExplainInRepos(
                                   CliGlobals * cliGlobals,
                                   Int64 * execStartUtcTs,
                                   char * qid, Lng32 qidLen,
                                   char * explainData, Lng32 explainDataLen);
  
private:

  // A reference to the cooresponding TDB (ExExplainTdb)
  inline ExExplainTdb &explainTdb() const;

  // The scanPred expression which is stored in the TDB.
  // This predicate will be applied to every tuple in the
  // tree that is being explained.  If it evaluates to TRUE,
  // the tuple is passed to the parent.
  inline ex_expr *getScanPred() const;

  // The params expressions which is stored in the TDB.
  // This contiguous move expression will be evaluated to populate
  // the paramsTuple with the values of the parameters to the
  // Explain function.
  inline ex_expr *getParamsExpr() const;

  // Method used to copy the parameters from the paramsTuple to 
  // the buffers (modName_ and stmtPattern_ see below).  These buffers
  // are allocated by the method initParamsTuple().
  void copyParameters();

  // Boolean used to determine if the value of the module name 
  // parameter stored in the paramsTuple is NULL.  Will return 1 if
  // the value is NULL 0 otherwise.
  short isNullModName();

  // Boolean used to determine if the value of the statement pattern 
  // parameter stored in the paramsTuple is NULL.  Will return 1 if
  // the value is NULL 0 otherwise.
  short isNullStmtPattern();

  // Method used to get a pointer to the explainTree of the statement
  // specified by the parameters (module name and statement pattern).
  // Since the statement pattern can contain wild card characters (as
  // with the LIKE predicate), there can be several statements that
  // match.  Repeated calls to this method will return each match.
  // When no more matches exist, NULL is returned.
  ExplainDesc *getNextExplainTree();

  // explain data is at explainFragAddr. Unpack it and return
  ExplainDesc * getNextExplainTree(Int64 explainFragAddr);

  // Method used to  traverse the Explain tree.  Enough state must be kept
  // in the TCB so that if necessary the routine can return and later be
  // restarted where it left off.  The routine will return if the traversal
  // is complete or if the parent up queue is full.
  traverseReturnCode traverseTree();

  // This routine is called by traverseTree() on each node of the
  // explainTree.  visitNode will check to make sure that this node has
  // been initialized and has been populated with explain Info.  If it has,
  // it will then apply the scan predicate (if it exists) to the node and
  // if true will insert the explain Tuple into the parent up queue.

  Int32 visitNode();

  // loadModule loads the named module if it has not already been loaded.
  // assumes that the buffer modName_ has been allocated and contains the
  // name of the module calculated by evaluating the paramsExpr.  Also,
  // initialize the statement list (stmtList_) to point to the statement
  // list of the current context.  The context is only available from the
  // master executor.

  Int32 loadModule();
  
  short processExplainStmt();
  short processExplainPlan();

  short getExplainFromRepos(char * qid, Lng32 qidLen);

  // private state

  // Queues used to communicate with the parent TCB.
  ex_queue_pair  qParent_;

  // The Params Atp (and other supporting stuctures):
  //  The last entry in the Params Atp will point to paramsTupp_.
  //  paramsTupp_ will point to paramsTuppDesc_.
  //  paramsTuppDesc_ will point to paramsTuple_.
  //  paramsTuple_ will point to a buffer allocated to hold
  //  the result of applying the paramsExpr_.
  //  This is all set up by the method initParamsTuple();

  atp_struct * paramsAtp_;
  tupp paramsTupp_;
  tupp_descriptor paramsTuppDesc_;
  char *paramsTuple_;

  // Pointer to buffer used to hold the module name parameter.
  // The buffer is allocated by inintParamsTuple().
  // The value of the parameter is copied to this buffer from the 
  // paramsTuple_ by the method copyParameters().
  char *modName_;

  // if the 3-part module name is prepended by the directory where this
  // module is present, then this parameter contains that dir name.
  // The dir name must be in the for /usr/application/.
  char *modDir_;

  // Pointer to buffer used to hold the statement pattern parameter.
  // The buffer is allocated by inintParamsTuple().
  // The value of the parameter is copied to this buffer from the 
  // paramsTuple_ by the method copyParameters().
  char *stmtPattern_;

  // A list of statements for the module named by the module name
  // parameter.
  HashQueue *stmtList_;

  // A pointer to the statement whose explainTree is currently
  // being traversed.
  Statement *currentStmt_;

  // A pointer to the root of the explainTree that is currently
  // being explained.
  ExplainDesc *explainTree_;

  // A pointer to the node of the explainTree which is currently
  // being explained.  This is part of the state kept by traverseTree().
  ExplainTuple *currentExplain_;

  // A ponter to a node of the explainTree.  This will be a pointer
  // to the child node which was most recently visited,  Or NULL when
  // the next child node is to be visited next (see ex_explain.C for
  // more details
  ExplainTuple *cameFrom_;

  // An enumeration of the nodes of the explainTree.  This is maintained
  // by the traverseTree method.  This value is used to set the 'seq_num'
  // field of the explainTuple.
  UInt32 seqNum_;

  // Number of rows returned to the parent.  Used to interface with
  // the parent up queue.
  UInt32 matchNo_;

  // Used to test the work procedure.  It is used to simulate a
  // FULL queue every so often.
  UInt32 debugCounter_;

  // Possible states of the work procedure.
  enum explain_work_state {
    EXPL_IDLE,
    EXPL_GET_PARAMS,
    EXPL_GETNEXT_EXPLAINTREE,
    EXPL_TRAVERSE_EXPLAINTREE,
    EXPL_ERROR,
    EXPL_DONE,
    EXPL_SEND_TO_SSMP,
    };

  // The current state of the work procedure.
  explain_work_state workState_;

  // Contains the queryId, if stmtPattern_ starts with QID=
  char *qid_;

  // query id of explain information that will be read from repository.
  char * reposQid_;

  Int64 explainAddr_;
  NABoolean explainFromAddrProcessed_;

  char * explainStmt_;
  char * explainPlan_;
  Lng32 explainPlanLen_;
  char * explainFrag_;
  Lng32 explainFragLen_;
  
  ComDiagsArea * diagsArea_;
  Lng32 retryAttempts_;
  char *stmtName_;
};

//  Inline procedures

inline ExExplainTdb &
ExExplainTcb::explainTdb() const
{
  return (ExExplainTdb &) tdb;
};

inline ex_expr *
ExExplainTcb::getScanPred() const
{
  return explainTdb().getScanPred();
};

inline ex_expr *
ExExplainTcb::getParamsExpr() const
{
  return explainTdb().getParamsExpr();
};

#endif


