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
//
**********************************************************************/
#ifndef SQLCI_RW_CMD_H
#define SQLCI_RW_CMD_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         SqlciRWCmd.h
 * Description:  Methods to process commands that interact with SQL/CLI and RW.
 *               
 *               
 * Created:      6/6/2003
 * Language:     C++
 * Status:       
 *
 *
 *
 *
 *****************************************************************************
 */

#include "SqlciNode.h"
#include "RWInterface.h"
#include "SqlciEnv.h"
#include "SqlciError.h"

// forward declarations to reduce header file recompilations.
class PrepStmt;

//////////////////////////////////////////
// THis method handles error for report writer
// interface related mehods.
///////////////////////////////////////////
void ReportWriterError ( ErrorValue *e); 
void SetError ( ErrorValue *e, Lng32 errorCode);

///////////////////////////////////////////////////////////////////////
//
// This is the base class for RW commands.
// This class is not constructed by the parser.
//
///////////////////////////////////////////////////////////////////////

class SqlciRWCmd : public SqlciNode {
public:
  enum report_cmd_type {
        LIST_TYPE,CANCEL_TYPE, SELECT_TYPE, EXECUTE_TYPE, RW_QUERY_TYPE
  };
  SqlciRWCmd (report_cmd_type cmd_type_);
  ~SqlciRWCmd();
  NABoolean isAllowedInSIP() { return TRUE;};
  NABoolean isAllowedInRWMode() { return FALSE; };
  NABoolean isAllowedInCSMode() { return FALSE; };


private:
  report_cmd_type cmd_type;

};

///////////////////////////////////////////////////////////////////
// this class is created by sqlci parser when a LIST command
// ("LIST FIRST <N>", ...etc) is seen.
//
// The class is constructed with the list type and the number of
// rows specified in it.
//
// ListType values:
// List First:  FIRST_
// List Next:   NEXT_
// List All:    ALL_
//
// numRows: number of rows specified in the List command.
//          -1, if 'list all' is specified.
//
//////////////////////////////////////////////////////////////////
class SqlciRWListCmd : public SqlciRWCmd
{
public:
  enum ListType
  {
    FIRST_, NEXT_, ALL_
  };

  SqlciRWListCmd(ListType type, Lng32 listCount, NABoolean listCountSet);
  ~SqlciRWListCmd();
  short process(SqlciEnv * sqlci_env);

private:
  ListType type_;
  Lng32 listCount_;
  NABoolean listCountSet_;
};

//////////////////////////////////////////////////////////////////////////////
//
// This class is constructed by the parser when a CANCEL is seen.
// This cancels any select in progress command, then gets out of RW
// prompt and into mxci prompt.
//
//////////////////////////////////////////////////////////////////////////////
class SqlciRWCancelCmd : public SqlciRWCmd
{
public:
  SqlciRWCancelCmd();
  ~SqlciRWCancelCmd();
  short process(SqlciEnv * sqlci_env);

};


//////////////////////////////////////////////////////////////////////////////
//
// This class is constructed by the parser when a report writer query is seen.
// These are the queries which are to be passed to RW without any
// interpretation by sqlci.
//
//////////////////////////////////////////////////////////////////////////////
class SqlciRWQueryCmd : public SqlciRWCmd
{
public:
  SqlciRWQueryCmd(char * rwCmd, Lng32 rwCmdLen_);
  ~SqlciRWQueryCmd();
  short process(SqlciEnv * sqlci_env);
  NABoolean isAllowedInRWMode() { return TRUE; };

private:
  char * rwCmd_;
  Lng32   rwCmdLen_;
};

////////////////////////////////////////////////////////////////////
//
// This class is constructed by parser when a RW Select command is
// seen. This is the select query that follows a 'set list_count'
// command.
//
////////////////////////////////////////////////////////////////////
class SqlciRWSelectCmd : public SqlciRWCmd
{
public:
  SqlciRWSelectCmd(char * selectCmd);
  ~SqlciRWSelectCmd();
  short process(SqlciEnv * sqlci_env);
  NABoolean isAllowedInRWMode() { return TRUE; };

private:
  char * selectCmd_;
};

////////////////////////////////////////////////////////////////////
//
// This class is constructed by parser when a RW execute command is
// seen. This is the execute of a prepared select query that follows 
// a 'set list_count' command.
//
////////////////////////////////////////////////////////////////////
class SqlciRWExecuteCmd : public SqlciRWCmd
{
public:
  SqlciRWExecuteCmd(char * stmtName, char * usingParamStr);
  ~SqlciRWExecuteCmd();
  short process(SqlciEnv * sqlci_env);
  NABoolean isAllowedInRWMode() { return TRUE; };

private:
  char * stmtName_;

  char * usingParamStr_;
  char ** usingParams_; // [MAX_NUM_UNNAMED_PARAMS];
  short numUsingParams_;
};

////////////////////////////////////////////////////////////////////////
//
// This class is the execution engine to process the RW select command.
// It contains all the methods and other information that is needed to
// interact with RW (mxci calling RW) when a select command is processed.
//
// It is constructed during SqlciRWSelectCmd::process and is 'alive'
// as long as that 'select is in progress'. The globals RWEnv points to this
// class.
//
////////////////////////////////////////////////////////////////////////

class SqlciRWInterfaceExecutor
{
public:
  enum ExecutionState
  {
    INITIAL_,
    PREPARE_, OPEN_AND_FETCH_, OPEN_AND_EXIT_, FETCH_, CLOSE_,
    SELECT_STARTED_,
    LIST_FIRST_, LIST_NEXT_, LIST_ALL_, RESET_LIST_,
    GET_INPUT_FROM_SQLCI_, SEND_QUERY_TO_RW_,
    SEND_FETCHED_ROW_TO_RW_, GET_OUTPUT_ROW_FROM_RW_,
    PRINT_OUTPUT_,
    CANCEL_,
    LIST_ENDED_, SELECT_ENDED_,
    SQL_ERROR_, RW_ERROR_, 
    EXIT_
  };

  SqlciRWInterfaceExecutor();

  ~SqlciRWInterfaceExecutor();

  short process(SqlciEnv * sqlci_env);

  void setState(ExecutionState state) { state_ = state; };

  void setListCount(Lng32 listCount) 
  { 
    listCount_ = listCount; 
    currCount_ = -1;
  };

  void setSelectCmd(char * selectCmd);
  void setExecuteCmd(char * executeStmtName);
  void setExecutePrepStmt(PrepStmt * prepStmt)
  {
    prepStmt_ = prepStmt;
  }

  short setUsingParamInfo(char * usingParamStr);
  void setRWCmd(char * rwCmd, Lng32 rwCmdLen);

  ExecutionState getNextState(Lng32 retcode, ExecutionState nextState);

  Lng32 printOutputRow(SqlciEnv * sqlci_env, char * outputBuf, Lng32 outputBufLen);

  PrepStmt * prepStmt() { return prepStmt_; };
  void Close(SqlciEnv * sqlci_env); // Closes the prepStmt and deallocates/
  
  NABoolean resetListFlag() { return (flags_ & RESET_LIST_FLAG) != 0; }
  void setResetListFlag(NABoolean v)
  { (v ? flags_ |= RESET_LIST_FLAG : flags_ &= ~RESET_LIST_FLAG); }

private:
  enum {
    RESET_LIST_FLAG = 0x0001
  };

  char * selectCmd_;

  char * executeStmtName_;
  char ** usingParams_;
  short numUsingParams_;

  char * rwCmd_;
  Lng32   rwCmdLen_;

  char * inputRow_;
  Lng32   inputRowLen_;

  char * outputRow_;
  Lng32   outputRowLen_;

  Lng32 listCount_;
  Lng32 currCount_;

  NABoolean cursorOpened_;

  PrepStmt * prepStmt_;

  ULng32 flags_;

  ExecutionState state_;

  // private methods
  short open(SqlciEnv* sqlci_env);

};


#endif

