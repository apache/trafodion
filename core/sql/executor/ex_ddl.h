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
#ifndef EX_DDL_H
#define EX_DDL_H


/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ex_ddl.h
 * Description:  DDL statements (get executed in mxcmp)
 *               
 *               
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// forward
class ex_expr;

#include "ComTdbDDL.h"
#include "ComVersionDefs.h"
#include "ex_exe_stmt_globals.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ExDDLTdb;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ExDDLTdb
// -----------------------------------------------------------------------
class ExDDLTdb : public ComTdbDDL
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExDDLTdb()
  {}

  virtual ~ExDDLTdb()
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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};


// -----------------------------------------------------------------------
// ExDDLwithStatusTdb
// -----------------------------------------------------------------------
class ExDDLwithStatusTdb : public ComTdbDDLwithStatus
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExDDLwithStatusTdb()
  {}

  virtual ~ExDDLwithStatusTdb()
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
  //    If yes, put them in the ComTdbDLL instead.
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
class ExDDLTcb : public ex_tcb
{
  friend class ExDDLTdb;
  friend class ExDDLPrivateState;

public:
  enum Step
  {
    EMPTY_,
    REQUEST_SENT_,
    RETURNING_DATA_,
    DONE_,
    HANDLE_ERROR_,
    CANCELLED_,
    RETURNING_LEAKS_
  };

  // Constructor
  ExDDLTcb(const ComTdbDDL & ddl_tdb,
	   ex_globals * glob = 0);

  ~ExDDLTcb();

  virtual short work();

  ex_queue_pair getParentQueue() const;
  Int32 orderedQueueProtocol() const;

  void freeResources();

  Int32 numChildren() const;
  const ex_tcb* getChild(Int32 pos) const;

protected:
  void handleErrors(ex_queue_entry *pentry_down, ComDiagsArea *da, Int32 error);

  ex_queue_pair	qparent_;

  unsigned short tcbFlags_;

  atp_struct * workAtp_;

  // VO, plan versioning support
  // Some operations, like showshape, may require a downrev compiler.
  // This is not detected until execution time though. 
  COM_VERSION compilerVersion_;

  const inline COM_VERSION getCompilerVersion (void) const { return compilerVersion_; };
  inline void setCompilerVersion (COM_VERSION version)     { compilerVersion_ = version; };
  ExSqlComp * getArkcmp (void);

  // Return the CLI context
  ContextCli * currContext (void)
    { return getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals()->getCliGlobals()->currContext(); };

  inline ExDDLTdb & ddlTdb() const{return (ExDDLTdb &) tdb;};
};

//
// Task control block
//
class ExDDLwithStatusTcb : public ExDDLTcb
{
  friend class ExDDLTdb;
  friend class ExDDLPrivateState;

public:
  enum Step
  {
    NOT_STARTED_,
    SETUP_INITIAL_REQ_,
    CALL_EMBEDDED_CMP_,
    SEND_REQ_TO_CMP_,
    PROCESS_REPLY_,
    SETUP_NEXT_STEP_,
    RETURN_STATUS_,
    RETURN_DETAILS_,
    RETURN_STATUS_END_STEP_,
    DONE_,
    HANDLE_ERROR_,
    CANCELLED_
  };

  // Constructor
  ExDDLwithStatusTcb(const ComTdbDDL & ddl_tdb,
                     ex_globals * glob = 0);

  ~ExDDLwithStatusTcb()
    {}

  virtual short work();

  ComDiagsArea * getDiagsArea() { return diagsArea_; }
  inline ExDDLwithStatusTdb & ddlTdb() const{return (ExDDLwithStatusTdb &) tdb;};

 private:
  Step step_;
  Lng32 ddlStep_;
  Lng32 ddlSubstep_;

  char * upgdMsg_;

  ExSqlComp * cmp_;
  char * replyBuf_;
  ULng32 replyBufLen_;

  CmpDDLwithStatusInfo * mdi_;
  CmpDDLwithStatusInfo * replyDWS_;

  Int64 startTime_;
  Int64 endTime_;
  Int64 queryStartTime_;

  char * data_;
  size_t dataLen_;

  NABoolean callEmbeddedCmp_;

  Int32 numEntries_;
  Int32 currEntry_;
  char * currPtr_;

  ComDiagsArea * diagsArea_;
};

class ExDDLPrivateState : public ex_tcb_private_state
{
  friend class ExDDLTcb;
  friend class ExDDLwithStatusTcb;
  friend class ExDescribeTcb;
  
public:	
  ExDDLPrivateState(const ExDDLTcb * tcb); //constructor
  ~ExDDLPrivateState();	// destructor
  ex_tcb_private_state * allocate_new(const ex_tcb * tcb);
protected:
  void init();
  ExDDLTcb::Step step_;
  Int64 matches_;

  // the request and reply objects used to send and receive
  // data from arkcmp.
  void * request_;
  void * reply_;

  char * dataPtr_;
  ULng32 dataLen_;
  ULng32 currLen_;
};


////////////////////////////////////////////////////////////////////
// classes ExDescribeTdb, ExDescribeTcb, ExDescribePrivateState
////////////////////////////////////////////////////////////////////
#include "ComTdbDDL.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ExDescribeTdb;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;

// -----------------------------------------------------------------------
// ExDescribeTdb
// -----------------------------------------------------------------------
class ExDescribeTdb : public ComTdbDescribe
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExDescribeTdb()
  {}

  virtual ~ExDescribeTdb()
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
  //    If yes, put them in the ComTdbDescribe instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};


class ExDescribeTcb : public ExDDLTcb
{
public:
  ExDescribeTcb(const ExDescribeTdb & describe_tdb,
		ex_globals * glob = 0);

  short work();

private:

  inline ExDescribeTdb & describeTdb() const{return (ExDescribeTdb &) tdb;};

  Lng32 returnLeaks(short &error);
};

///////////////////////////////////////////////////////////////////
// This work method is defined in ex_control.cpp, alongwith other
// SHOWSET stmt methods.
///////////////////////////////////////////////////////////////////
class ExShowEnvvarsTcb : public ExDescribeTcb
{
public:
  ExShowEnvvarsTcb(const ExDescribeTdb & describe_tdb,
		   ex_globals * glob = 0);

  virtual short work();

protected:
  enum Step
  {
    EMPTY_,
    RETURN_HEADER_,
    RETURNING_VALUE_,
    DONE_,
    HANDLE_ERROR_,
    CANCELLED_
  };
  
  inline ExDescribeTdb & showTdb() const{return (ExDescribeTdb &) tdb;};

  short moveRowToUpQueue(Lng32 tuppIndex,
			 const char * row, 
			 Lng32 len = 0, 
			 short * rc = NULL);
    
private:
  Step step_;

  Lng32 currEnvvar_;
};

// -----------------------------------------------------------------------
// ExProcessVolatileTableTdb
// -----------------------------------------------------------------------
class ExProcessVolatileTableTdb : public ComTdbProcessVolatileTable
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExProcessVolatileTableTdb()
  {}

  virtual ~ExProcessVolatileTableTdb()
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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

class ExProcessVolatileTableTcb : public ExDDLTcb
{
  friend class ExProcessVolatileTableTdb;
  friend class ExExeUtilPrivateState;

public:
  // Constructor
  ExProcessVolatileTableTcb(const ComTdbProcessVolatileTable & exe_util_tdb,
			    ex_globals * glob = 0);

  virtual short work();

  ExProcessVolatileTableTdb & pvtTdb() const
  {
    return (ExProcessVolatileTableTdb &) tdb;
  };

  virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element

private:
  enum Step
  {
    INITIAL_,
    SEND_DDL_EXPR_,
    ADD_TO_VOL_TAB_LIST_,
    REMOVE_FROM_VOL_TAB_LIST_,
    CREATE_VOLATILE_SCHEMA_,
    SET_VOLATILE_SCHEMA_USAGE_CQD_,
    RESET_VOLATILE_SCHEMA_USAGE_CQD_,
    ERROR_,
    DONE_
  };

  Step step_;

};

class ExProcessVolatileTablePrivateState : public ex_tcb_private_state
{
  friend class ExProcessVolatileTableTcb;
  
public:	
  ExProcessVolatileTablePrivateState();
  ~ExProcessVolatileTablePrivateState();	// destructor
protected:
};

//////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------
// ExProcessInMemoryTableTdb
// -----------------------------------------------------------------------
class ExProcessInMemoryTableTdb : public ComTdbProcessInMemoryTable
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExProcessInMemoryTableTdb()
  {}

  virtual ~ExProcessInMemoryTableTdb()
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
  //    If yes, put them in the ComTdbDLL instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

class ExProcessInMemoryTableTcb : public ExDDLTcb
{
  friend class ExProcessInMemoryTableTdb;
  friend class ExExeUtilPrivateState;

public:
  // Constructor
  ExProcessInMemoryTableTcb(const ComTdbProcessInMemoryTable & exe_util_tdb,
			    ex_globals * glob = 0);

  virtual short work();

  ExProcessInMemoryTableTdb & pimtTdb() const
  {
    return (ExProcessInMemoryTableTdb &) tdb;
  };

  virtual ex_tcb_private_state * allocatePstates(
       Lng32 &numElems,      // inout, desired/actual elements
       Lng32 &pstateLength); // out, length of one element

private:
  enum Step
  {
    INITIAL_,
    CREATE_VOLATILE_SCHEMA_,
    SET_VOLATILE_SCHEMA_USAGE_CQD_,
    TURN_QUERY_CACHE_OFF_,
    SEND_DDL_EXPR_,
    DROP_,
    ERROR_,
    DONE_
  };

  Step step_;

  NABoolean volSchCreatedHere_;
};

class ExProcessInMemoryTablePrivateState : public ex_tcb_private_state
{
  friend class ExProcessInMemoryTableTcb;
  
public:	
  ExProcessInMemoryTablePrivateState();
  ~ExProcessInMemoryTablePrivateState();	// destructor
protected:
};



#endif



