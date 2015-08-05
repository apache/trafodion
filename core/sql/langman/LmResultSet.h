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
/* -*-C++-*-
**********************************************************************
*
* File:         LmResultSet.h
* Description:  Container base class for result set data.
*
* Created:      06/14/2005
* Language:     C++
*
**********************************************************************
*/

#ifndef LMRESULTSET_H
#define LMRESULTSET_H

#include "ComSmallDefs.h"
#include "LmCommon.h"
#include "sqlcli.h"

class LmParameter;
//////////////////////////////////////////////////////////////////////
//
// Contents
//
//////////////////////////////////////////////////////////////////////
class LmResultSet;

//////////////////////////////////////////////////////////////////////
//
// LmResultSet
//
//////////////////////////////////////////////////////////////////////

class SQLLM_LIB_FUNC LmResultSet : public NABasicObject
{

public:
  // Gets the pointer to the result set's SQLSTMT_ID CLI structure.
  SQLSTMT_ID *getStmtID() const { return stmtID_; }

  NABoolean isCliStmtAvailable() { return stmtID_ != NULL; }

  // Gets the CLI Context Handle containing the result set cursor
  SQLCTX_HANDLE getCtxHandle() const { return ctxHandle_; }

  // Destructor
  virtual ~LmResultSet() {};

  virtual NABoolean moreSpecialRows() { return 0; }

  // Gets the CLI statement status returned by JDBC
  virtual NABoolean isCLIStmtClosed() = 0;

  virtual char *getProxySyntax() = 0;

  // Gets a row from JDBC. It can not get a row from CLI.
  virtual Lng32 fetchSpecialRows(void *, LmParameter *, ComUInt32, 
                                ComDiagsArea &, ComDiagsArea *) = 0; 

protected:
  // Constructor
  LmResultSet() : ctxHandle_(0), stmtID_(NULL)
  {}

  // Mutator methods
  void setStmtID( SQLSTMT_ID *stmtID ) { stmtID_ = stmtID; }
  void setCtxHandle( SQLCTX_HANDLE ctxHndl ) { ctxHandle_ = ctxHndl; }

private:
  SQLCTX_HANDLE ctxHandle_;	// CLI Context Handle containing the result set cursor
  SQLSTMT_ID *stmtID_;		// Pointer to result set's SQLSTMT_ID CLI structure.

};

#endif
