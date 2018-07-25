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
#ifndef SQLCINODE_H
#define SQLCINODE_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         SqlciNode.h
 * Description:  
 * Created:      4/15/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"
#include "SqlCliDllDefines.h"
#include "NABoolean.h"
#include <string.h>

class SqlciEnv;

class SqlciNode {
public:
enum sqlci_node_type {
  SQLCI_CMD_TYPE, SQL_CMD_TYPE, UTIL_CMD_TYPE, SHELL_CMD_TYPE, SQLCLI_CMD_TYPE, REPORT_CMD_TYPE, 
  MXCS_CMD_TYPE
};

private:
  char eye_catcher[4];
  sqlci_node_type node_type;
  SqlciNode * next;
  Lng32 errcode;

public:
  SqlciNode(const sqlci_node_type);
  virtual ~SqlciNode();
  virtual short process(SqlciEnv * sqlci_env);

  void set_next(SqlciNode * next_)	{next = next_;}
  SqlciNode * get_next() const		{return next;}

  Lng32 errorCode() const		{return errcode;}
  void setErrorCode(Lng32 e)		{errcode = e;}

  bool isSqlciNode() const              {return strncmp(eye_catcher, "CI  ", 4) == 0;}

  sqlci_node_type getType()             {return node_type;}

};

#endif



