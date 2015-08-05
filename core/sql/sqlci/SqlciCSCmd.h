#ifndef SQLCI_CS_CMD_H
#define SQLCI_CS_CMD_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         SqlciCSCmd.h
 * Description:  Methods to process commands that interact with MXCI and MACL
 *               
 *               
 * Created:      11/17/2003
 * Language:     C++
 * Status:       
 *
 *
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
 *
 *
 *****************************************************************************
 */

#include "SqlciNode.h"
#include "CSInterface.h"
#include "SqlciEnv.h"
#include "SqlciError.h"

//////////////////////////////////////////
// THis method handles error for MACL
// interface related mehods.
///////////////////////////////////////////
void MXCSError ( CSErrorValue *e); 
void SetMXCSError ( CSErrorValue *e, Lng32 errorCode);

///////////////////////////////////////////////////////////////////////
//
// This is the base class for CS commands.
// This class is not constructed by the parser.
//
///////////////////////////////////////////////////////////////////////

class SqlciCSCmd : public SqlciNode {
public:
  enum cs_cmd_type {
       CS_QUERY_TYPE
  };
  SqlciCSCmd (cs_cmd_type cmd_type_);
  ~SqlciCSCmd();
  NABoolean isAllowedInSIPMode() { return FALSE; };
  NABoolean isAllowedInRWMode() { return FALSE; };
  NABoolean isAllowedInCSMode() { return FALSE; };


private:
   cs_cmd_type cmd_type;

};



//////////////////////////////////////////////////////////////////////////////
//
// This class is constructed by the parser when a MXCS query is seen.
// These are the queries which are to be passed to MXCS without any
// interpretation by MXCI
//
//////////////////////////////////////////////////////////////////////////////
class SqlciCSQueryCmd : public SqlciCSCmd
{
public:
  SqlciCSQueryCmd(char * csCmd, Lng32 csCmdLen_);
  ~SqlciCSQueryCmd();
  short process(SqlciEnv * sqlci_env);
  NABoolean isAllowedInCSMode() { return TRUE; };

private:
  char * csCmd_;
  Lng32   csCmdLen_;
};



////////////////////////////////////////////////////////////////////////
//
// This class is the execution engine to process the MACL command.
// It contains all the methods and other information that is needed to
// interact with MACL 
//
// It is constructed during SqlciCSQueryCmd::process and is 'alive'
// as long as that 'select is in progress'. The globals CSEnv points to this
// class.
//
////////////////////////////////////////////////////////////////////////

class SqlciCSInterfaceExecutor
{
public:
  enum ExecutionState
  {
    INITIAL_,
    GET_INPUT_FROM_MXCI_, 
    SEND_QUERY_TO_CS_,
    GET_OUTPUT_ROW_FROM_CS_,
    PRINT_OUTPUT_,
    CS_ERROR_, 
    EXIT_
  };

  SqlciCSInterfaceExecutor();

  ~SqlciCSInterfaceExecutor();

  short process(SqlciEnv * sqlci_env);

  void setState(ExecutionState state) { state_ = state; };

  void setCSCmd(char * csCmd, Lng32 csCmdLen);

  ExecutionState getNextState(Lng32 retcode, ExecutionState nextState);

  Lng32 printOutputRow(SqlciEnv * sqlci_env, char * outputBuf, Lng32 outputBufLen);


private:


  char * csCmd_;
  Lng32   csCmdLen_;

  char * outputRow_;
  Lng32   outputRowLen_;

  ExecutionState state_;


};


#endif

