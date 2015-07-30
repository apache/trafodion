#ifndef STMTCOMPILATIONMODE_H
#define STMTCOMPILATIONMODE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtCompilationMode.h
 * Description:  class for parse node representing Static / Dynamic SQL
 *               Static allow host variable , but not param. Dynamic allow
 *               param, but not host variable.
 *
 * Created:      03/28/96
 * Language:     C++
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
 *****************************************************************************
 */

#include "NABasicObject.h"

enum WhoAmI { I_AM_UNKNOWN,
	      I_AM_C_PREPROCESSOR, I_AM_COBOL_PREPROCESSOR,
              I_AM_ESP,
	      I_AM_SQL_COMPILER, I_AM_EMBEDDED_SQL_COMPILER };

struct IdentifyMyself
{
public:
  static WhoAmI GetMyName()			{ return myName_; }
  static void SetMyName(WhoAmI myname)		{ myName_ = myname; }

  static NABoolean IsPreprocessor()
  { return ( GetMyName() == I_AM_COBOL_PREPROCESSOR ||
             GetMyName() == I_AM_C_PREPROCESSOR );
  };

private:
  static THREAD_P WhoAmI myName_;
};

#endif // STMTCOMPILATIONMODE_H
