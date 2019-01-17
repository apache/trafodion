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
#ifndef EXPORTFUNCTIONSQLCMPDBG_H
#define EXPORTFUNCTIONSQLCMPDBG_H

#include <iostream>
using namespace std;

#include "CommonSqlCmpDbg.h"
extern "C" Q_DECL_EXPORT SqlcmpdbgExpFuncs * GetSqlcmpdbgExpFuncs ();
extern "C" Q_DECL_EXPORT void DisplayQueryTree (Sqlcmpdbg::CompilationPhase
						phase, void *tree =
						NULL, void *plan = NULL);
extern "C" Q_DECL_EXPORT void SqldbgSetCmpPointers (void *memo, void *tasklist,
                                                    void *analysis,
                                                    void *currentContext,
                                                    void *ClusterInfo);
extern "C" Q_DECL_EXPORT void DoMemoStep (Int32 passNo = -1, Int32 groupNo =
					  -1, Int32 taskNo = -1, void *task =
					  NULL, void *expr =
					  NULL, void *plan = NULL);
extern "C" Q_DECL_EXPORT void HideQueryTree (BOOL flag = TRUE);
extern "C" Q_DECL_EXPORT void DisplayTDBTree (Sqlcmpdbg::CompilationPhase
					      phase, void *tdb,
					      void *fragDir);
extern "C" Q_DECL_EXPORT int  ExecutionDisplayIsEnabled (void);
extern "C" Q_DECL_EXPORT void SqldbgSetExePointers (void *rootTcb,
                                                    void *cliGlobals,
                                                    void *dummy);
extern "C" Q_DECL_EXPORT void DisplayExecution (ExSubtask**, ExScheduler *);

extern "C" Q_DECL_EXPORT void CleanUp(void);
#endif // EXPORTFUNCTIONSQLCMPDBG_H
