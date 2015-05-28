// **********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2009-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
// **********************************************************************
#ifndef CMPCLICALLS_H
#define CMPCLICALLS_H

#include "NAString.h"
#include "CmpCommon.h"
#include "SchemaDB.h"
#include "cli_stdh.h"
#include "sql_id.h"
#include "charinfo.h"

/************************************************************************
class CmpCliStmt

Used to make CLI calls to the executor (modeled after CatCliStmt
from catman)

************************************************************************/
class CmpCliStmt
{
public:

  CmpCliStmt( const char *dml, const CharInfo::CharSet dmlStmtCharSet = CharInfo::UTF8);

  virtual ~CmpCliStmt();
  
  NABoolean executeNoDataExpected();  
  void myfree();
  void mergeErrors();

protected:
  void handleError(Lng32 error);

private:
  CmpCliStmt();  // do not use
  CmpCliStmt( const CmpCliStmt &copy ); // do not use

  SQLSTMT_ID         stmtId_;
  SQLMODULE_ID       moduleId_;
  SQLDESC_ID         srcDesc_;
};
#endif // CMPCLICALLS_H
