#ifndef SQLCIDEFS_H
#define SQLCIDEFS_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         SqlciDefs.h
 * RCS:          $Id: SqlciDefs.h,v 1.6 1998/07/10 00:51:45  Exp $
 * Description:  
 *               
 *               
 * Created:      4/15/95
 * Modified:     $ $Date: 1998/07/10 00:51:45 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
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

#include "Platform.h"

enum dml_type 
{
  DML_CONTROL_TYPE, 
  DML_SELECT_TYPE,
  DML_UPDATE_TYPE,
  DML_INSERT_TYPE,
  DML_DELETE_TYPE,
  DML_DDL_TYPE,
  DML_DESCRIBE_TYPE,
  DML_SHOWSHAPE_TYPE,
  DML_DISPLAY_NO_ROWS_TYPE,	// special-purpose
  DML_DISPLAY_NO_HEADING_TYPE,	// special-purpose
  // $$$$ SPJ RS THROWAWAY
  DML_CALL_STMT_TYPE, // $$$$ Might not even need this one
  DML_CALL_STMT_RS_TYPE,
  DML_UNLOAD_TYPE,
  DML_OSIM_TYPE
};

enum SQLCI_CLI_RETCODE
{
  SQL_Success = 0, SQL_Eof = 100, SQL_Error = -1, SQL_Warning = 1, SQL_Canceled = -8007,
    SQL_Rejected = -15026

};

// A simple structure used by the sqlci parser to hold information 
// about a cursor
struct SqlciCursorInfo
{
  Int32 queryTextSpecified_;
  char *queryTextOrStmtName_;
  Lng32 resultSetIndex_;
  SqlciCursorInfo()
  {
    queryTextSpecified_ = 0;
    queryTextOrStmtName_ = 0;
    resultSetIndex_ = 0;
  }
};

#endif
