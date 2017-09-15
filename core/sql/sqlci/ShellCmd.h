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
#ifndef SHELLCMD_H
#define SHELLCMD_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ShellCmd.h
 * RCS:          $Id: ShellCmd.h,v 1.4 1997/07/12 18:02:34  Exp $
 * Description:
 *
 *
 * Created:      4/15/95
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 *****************************************************************************
 */


#include "SqlciNode.h"
#include "SqlciEnv.h"

class ShellCmd : public SqlciNode {
public:
enum shell_cmd_type {
  CD_TYPE, LS_TYPE, SHELL_TYPE
};

private:
  shell_cmd_type  cmd_type;
  char * argument;

public:
  ShellCmd(const shell_cmd_type cmd_type_, char * argument_);
  ~ShellCmd();
  inline char * get_argument(){return argument;};
};

class Chdir : public ShellCmd {
public:
  Chdir(char * argument_);
  ~Chdir(){};
  short process(SqlciEnv * sqlci_env);
};

class Ls : public ShellCmd {
public:
  Ls(char *argument_) ;
  ~Ls(){};
  short process(SqlciEnv * sqlci_env);
};

class Shell : public ShellCmd {
public:
  Shell(char * argument_);
  ~Shell(){};
  short process(SqlciEnv * sqlci_env);
};

#endif
