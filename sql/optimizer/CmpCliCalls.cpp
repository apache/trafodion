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
#include "CmpCliCalls.h"
/************************************************************************
constructor CmpCliStmt

Initialize the stmtId, moduleId_, and srcDesc_

************************************************************************/
CmpCliStmt::CmpCliStmt( const char *dml , const CharInfo::CharSet dmlStmtCharSet)

{
  Lng32 retcode;

  init_SQLMODULE_ID(&moduleId_);
  init_SQLCLI_OBJ_ID(&stmtId_);
  init_SQLCLI_OBJ_ID(&srcDesc_);

  stmtId_.version           = SQLCLI_CURRENT_VERSION;
  stmtId_.name_mode         = stmt_handle;
  stmtId_.module            = &moduleId_;  
  stmtId_.identifier        = 0;
  stmtId_.handle            = 0;
  moduleId_.module_name     = 0;
  moduleId_.module_name_len = 0;

  retcode = SQL_EXEC_ClearDiagnostics(&stmtId_);
  handleError(retcode);
  
  retcode = SQL_EXEC_AllocStmt(&stmtId_, 0);
  handleError(retcode);
  
  // Allocate a descriptor which will hold the SQL statement source
  srcDesc_.version        = SQLCLI_CURRENT_VERSION;
  srcDesc_.name_mode      = desc_handle;
  srcDesc_.module         = &moduleId_;
  srcDesc_.identifier     = 0;
  srcDesc_.handle         = 0;
  retcode = SQL_EXEC_AllocDesc(&srcDesc_, (SQLDESC_ID *)0);
  handleError(retcode);

  retcode = SQL_EXEC_SetDescItem(&srcDesc_, 
                                 1, 
                                 SQLDESC_TYPE_FS,
                                 (Lng32)REC_BYTE_V_ANSI, 
                                 0);
  handleError(retcode);

  retcode = SQL_EXEC_SetDescItem(&srcDesc_, 
                                  1, 
                                  SQLDESC_VAR_PTR,
                                 (Long)dml, 
                                 0);
  handleError(retcode);

  retcode = SQL_EXEC_SetDescItem(&srcDesc_, 
                                  1, 
                                  SQLDESC_LENGTH,
                                  (Lng32)(strlen(dml) + 1), 
                                  0);
  handleError(retcode);


  // SQLDESC_CHAR_SET must be the last descriptor item set, otherwise
  // it may get reset by other calls to SQL_EXEC_SetDescItem().
  NAString charSet = ActiveSchemaDB()->getDefaults().getValue(ISO_MAPPING);
  NAString defCS   = ActiveSchemaDB()->getDefaults().getValue(DEFAULT_CHARSET);
  retcode = SQL_EXEC_SetDescItem(&srcDesc_, 
                                  1, 
                                  SQLDESC_CHAR_SET,
                                  dmlStmtCharSet
                                  ,
                                      0);  
  handleError(retcode);
}
/************************************************************************
destructor CmpCliStmt

************************************************************************/
CmpCliStmt::~CmpCliStmt()
{
  myfree();
}
/************************************************************************
method free

Deallocate the srcDesc and stmtId

************************************************************************/
void
CmpCliStmt::myfree()
{
  SQL_EXEC_DeallocDesc(&srcDesc_);
  SQL_EXEC_DeallocStmt(&stmtId_);
}
/************************************************************************
method CmpCliStmt::executeNoDataExpected

  Execute a the statement. Do not expect anything returned

************************************************************************/
NABoolean 
CmpCliStmt::executeNoDataExpected()
{
  Lng32 retcode;

  retcode = SQL_EXEC_SetStmtAttr(&stmtId_, 
                                 SQL_ATTR_PARENT_QID, 
                                 0, 
        (char *)CmpCommon::context()->sqlSession()->getParentQid()); 
  
  if( retcode >= 0 )
  {
    retcode = SQL_EXEC_ExecDirect(&stmtId_, &srcDesc_, 0, 0);
  }

  mergeErrors();

  // if there's an error return FALSE.
  // otherwise true
  return (0 <= retcode);
}
/************************************************************************
method CmpCliStmt::mergeErrors

  Merge diags and convert errors into warnings

************************************************************************/
void
CmpCliStmt::mergeErrors()
{
  ComDiagsArea diags(STMTHEAP);
  
  SQL_EXEC_MergeDiagnostics_Internal ( diags );
  //
  // convert errors from CmpCliStmt calls into warnings.
  NegateAllErrors(&diags);

  CmpCommon::diags()->mergeAfter(diags);
}
/************************************************************************
method CmpCliStmt::handleError

  handle any errors

************************************************************************/
inline
void
CmpCliStmt::handleError(Lng32 error)
{
  if( error < 0 )
  {
    CMPASSERT("Error setting up CmpCliStmt.");
  }
}
