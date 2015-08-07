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
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         SqlCliCmd.cpp
 * Description:  CLI commands objects to test context operations
 * Created:      12/8/2000
 * Language:     C++
 *
 *
 *****************************************************************************
 */

#include "sqlclicmd.h"
#include "sqlcli.h"
#include "SQLCLIdev.h"
#include "CliDefs.h"
#include "sqlcmd.h"

// SqlCliCmd's methods
SqlCliCmd::SqlCliCmd() : SqlciNode(SqlciNode::SQLCLI_CMD_TYPE) {}
SqlCliCmd::~SqlCliCmd() {}

// CheckViolation's methods
CheckViolation::CheckViolation() {}
CheckViolation::~CheckViolation() {}
short CheckViolation::process(SqlciEnv * sqlciEnv){
  Lng32 sqlViolation, xactViolation, xactWasAborted;
  Lng32 udrErrorFlags = 0;
  Lng32 retCode =  SQL_EXEC_GetUdrErrorFlags_Internal(&udrErrorFlags);
  if((enum RETCODE) retCode == SUCCESS){
    sqlViolation = udrErrorFlags & SQLUDR_SQL_VIOL;
    xactViolation = udrErrorFlags & SQLUDR_XACT_VIOL;
    xactWasAborted = udrErrorFlags & SQLUDR_XACT_ABORT; 
    cerr << "success -- " << endl;
    cout << "sqlViolation:" << (sqlViolation ? "YES" : "NO") << endl;
    cout << "xactViolation:" << (xactViolation ? "YES" : "NO") << endl;
    cout << "xactWasAborted:" << (xactWasAborted ? "YES" : "NO") << endl;
  }
  else{
    cerr << "error -- " << endl;
  }
  HandleCLIError(retCode, sqlciEnv);
  return 0;
}

// ResetViolation's methods
ResetViolation::ResetViolation(enum ComRoutineSQLAccess mode) : mode_(mode) {}
ResetViolation::~ResetViolation() {}
short ResetViolation::process(SqlciEnv * sqlciEnv){
  Lng32 firstRetCode = SQL_EXEC_SetUdrAttributes_Internal(mode_, 0/* maxRSets */);
  Lng32 secondRetCode = SQL_EXEC_ResetUdrErrorFlags_Internal();
  if(((enum RETCODE) firstRetCode == SUCCESS) &&
     ((enum RETCODE) secondRetCode == SUCCESS)){
    cerr << "success -- " << endl;
  }
  else{
    cerr << "error -- " << endl;
    if((enum RETCODE) firstRetCode != SUCCESS)
      HandleCLIError(firstRetCode, sqlciEnv);
    else
      HandleCLIError(secondRetCode, sqlciEnv);
  }
  return 0;
}

// CreateContext's methods
CreateContext::CreateContext() : noAutoXact_(FALSE) {}
CreateContext::CreateContext(Int32 noAutoXact) : noAutoXact_(noAutoXact) {}
CreateContext::~CreateContext() {}
short CreateContext::process(SqlciEnv * sqlciEnv)
{
  SQLCTX_HANDLE ctxhdl = 0;
  Lng32 retCode = SQL_EXEC_CreateContext(&ctxhdl, NULL, noAutoXact_);

  if((enum RETCODE) retCode == SUCCESS){
    cerr << "success -- new handle:" << ctxhdl << endl;
  }
  else{
    cerr << "error -- " << endl;
    HandleCLIError(retCode, sqlciEnv);
  }
  return 0;
}

// CurrentContext's methods
CurrentContext::CurrentContext() {}
CurrentContext::~CurrentContext() {}
short CurrentContext::process(SqlciEnv * sqlciEnv)
{
  SQLCTX_HANDLE ctxhdl = 0;
  Lng32 retCode = SQL_EXEC_CurrentContext(&ctxhdl);
  if((enum RETCODE) retCode == SUCCESS){
    cerr << "success -- current handle:" << ctxhdl << endl;
  }
  else{
    cerr << "error -- " << endl;
    HandleCLIError(retCode, sqlciEnv);
  }
  return 0;
}

// SwitchContext's methods
SwitchContext::SwitchContext(Lng32 ctxHandle) :
  ctxHdl_(ctxHandle) {}
SwitchContext::~SwitchContext() {}
short SwitchContext::process(SqlciEnv * sqlciEnv)
{
  SQLCTX_HANDLE ctxhdl = 0;
  Lng32 retCode = SQL_EXEC_SwitchContext(ctxHdl_, &ctxhdl);
  if((enum RETCODE) retCode == SUCCESS){
    cerr << "success -- previous handle: " << ctxhdl << endl;
  }
  else{
    cerr << "error --" << endl;
    HandleCLIError(retCode, sqlciEnv);
  }
  return 0;
}  

// DeleteContext's methods
DeleteContext::DeleteContext(Lng32 ctxHandle) :
  ctxHdl_(ctxHandle) {}
DeleteContext::~DeleteContext() {}
short DeleteContext::process(SqlciEnv * sqlciEnv)
{
  Lng32 retCode = SQL_EXEC_DeleteContext(ctxHdl_);
  if((enum RETCODE) retCode == SUCCESS){
    cerr << "success --" << endl;
  }
  else{
    cerr << "error --" << endl;
    HandleCLIError(retCode, sqlciEnv);
  }
  return 0;
}

// ResetContext's methods
ResetContext::ResetContext(Lng32 ctxHandle) :
  ctxHdl_(ctxHandle) {}
ResetContext::~ResetContext() {}
short ResetContext::process(SqlciEnv * sqlciEnv)
{
  Lng32 retCode = SQL_EXEC_ResetContext(ctxHdl_, NULL);
  if((enum RETCODE) retCode == SUCCESS){
    cerr << "success --" << endl;
  }
  else{
    cerr << "error --" << endl;
    HandleCLIError(retCode, sqlciEnv);
  }
  return 0;
}
