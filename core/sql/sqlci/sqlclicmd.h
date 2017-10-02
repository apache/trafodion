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
#ifndef _SQLCLICMD_H_
#define _SQLCLICMD_H_

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         SqlCliCmd.h
 * Description:  CLI commands objects to test context operations
 * Created:      12/7/2000
 * Language:     C++
 *
 *
 *****************************************************************************
 */

#include "NAType.h"
#include "SqlciNode.h"
#include "SqlciEnv.h"
#include "ComSmallDefs.h"

// abstract class for all SqlCliCmds
class SqlCliCmd : public SqlciNode {
public:
  SqlCliCmd();
  virtual ~SqlCliCmd();
  virtual short process(SqlciEnv * sqlci_env) = 0;
};

class CheckViolation : public SqlCliCmd {
public:
  CheckViolation();
  virtual ~CheckViolation();
  virtual short process(SqlciEnv * sqlci_env);
};

class ResetViolation : public SqlCliCmd {
private:
  enum ComRoutineSQLAccess mode_;
public:
  ResetViolation(enum ComRoutineSQLAccess mode);
  virtual ~ResetViolation();
  virtual short process(SqlciEnv * sqlci_env);
};

class CreateContext : public SqlCliCmd {
private:
  NABoolean noAutoXact_;
public:
  CreateContext();
  CreateContext(NABoolean noAutoXact);
  virtual ~CreateContext();
  virtual short process(SqlciEnv * sqlci_env);
};

class CurrentContext : public SqlCliCmd {
public:
  CurrentContext();
  virtual ~CurrentContext();
  virtual short process(SqlciEnv * sqlciEnv);
};

class SwitchContext : public SqlCliCmd {
public:
  SwitchContext(Lng32 ctxHandle);
  virtual ~SwitchContext();
  virtual short process(SqlciEnv * sqlci_env);
private:
  Lng32 ctxHdl_;
};

class DeleteContext : public SqlCliCmd {
public:
  DeleteContext(Lng32 ctxHandle);
  virtual ~DeleteContext();
  virtual short process(SqlciEnv * sqlci_env);
private:
  Lng32 ctxHdl_;
};
    
class ResetContext : public SqlCliCmd {
public:
  ResetContext(Lng32 ctxHandle);
  virtual ~ResetContext();
  virtual short process(SqlciEnv * sqlci_env);
private:
  Lng32 ctxHdl_;
};

#endif
