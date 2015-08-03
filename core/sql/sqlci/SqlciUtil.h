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
 * File:         SqlciUtil.h
 * RCS:          $Id: SqlciUtil.h,v 1.4 1997/07/12 18:02:40  Exp $
 * Description:  Declarations for utilities commands
 *
 *
 * Created:      1/10/95
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 *****************************************************************************
 */

#ifndef UTILCMD_H
#define UTILCMD_H

#include "SqlciNode.h"
#include "SqlciList_templ.h"

class PrepStmt;
class SqlCmd;
class IpcServer;

class UtilCmd : public SqlciNode {
public:
enum UtilCmdType {
  NONE
};

private:
  UtilCmdType commandType_;
  char *argument_;
public:
  UtilCmd(UtilCmdType, char * argument);
  ~UtilCmd(){};
  short process(SqlciEnv * sqlciEnv);
  NABoolean isAllowedInSIP() { return FALSE; } ;
  NABoolean isAllowedInRWMode() {return TRUE; };
  NABoolean isAllowedInCSMode() {return FALSE; } ;
protected:
  char * getArgument() { return argument_;}
};

#endif





