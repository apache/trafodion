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
#ifndef COMSQLCMPDBG_H
#define COMSQLCMPDBG_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComSqlcmpdbg.h
 * Description:  This file contains declarations common to arkcmp components 	
 *               and tdm_sqlcmpdbg, the GUI tool used to display query
 *		 compilation and execution.
 *
 * Created:      06/25/97
 * Modified:     $Author:
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"

class ExScheduler;
class ExSubtask;

class Sqlcmpdbg {
  // This class exists merely to give a nice naming scope for this enum
public:
  enum CompilationPhase { AFTER_PARSING,
			  AFTER_BINDING,
			  AFTER_TRANSFORMATION,
			  AFTER_NORMALIZATION,
			  AFTER_SEMANTIC_QUERY_OPTIMIZATION,
                          DURING_MVQR,
			  AFTER_ANALYZE,
			  AFTER_OPT1,
			  AFTER_OPT2,
			  AFTER_PRECODEGEN,
			  AFTER_CODEGEN,
			  AFTER_TDBGEN,
			  DURING_EXECUTION,
			  DURING_MEMOIZATION,
                          FROM_MSDEV
			 };
};

struct SqlcmpdbgExpFuncs {
  void (*fpDisplayQueryTree) (Sqlcmpdbg::CompilationPhase,void* , void* );
  void (*fpSqldbgSetCmpPointers) (void*, void*, void*, void*, void* );
  void (*fpDoMemoStep) (Int32, Int32, Int32, void*, void*, void*);
  void (*fpHideQueryTree) (BOOL);
  void (*fpDisplayTDBTree) (Sqlcmpdbg::CompilationPhase, void*, void*);
  int  (*fpExecutionDisplayIsEnabled) (void);
  void (*fpSqldbgSetExePointers) (void *, void *, void *);
  void (*fpDisplayExecution) (ExSubtask**, ExScheduler *);
  void (*fpCleanUp)(void);
};

typedef SqlcmpdbgExpFuncs* (*fpGetSqlcmpdbgExpFuncs) ();

#endif
	
