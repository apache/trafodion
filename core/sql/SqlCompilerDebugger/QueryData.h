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
#ifndef QUERYDATA_H
#define QUERYDATA_H
#include <iostream>
using namespace std;

#include "CommonSqlCmpDbg.h"
class QueryData
{
public:
  QueryData ();
  void GetData (void **tree, void **plan);
  void SetData (void *tree, void *plan);
  void ResetData (void);
  void SetMemoSteppingFlag (BOOL flag);
  void SetPhase (Sqlcmpdbg::CompilationPhase phase);
  Sqlcmpdbg::CompilationPhase GetPhase ();
  void SetMemo (void *memoptr);
  void *GetMemo (void);
  void SetTaskList (void *tasklist);
  void *GetTaskList ();
  void SetAnalysis (void *analysis);
  void *GetAnalysis ();
  void SetTDBData (void *tdb, void *fragDir);
  void GetTDBData (void **tdb, void **fragDir, Lng32 & baseAddr);
  void SetCurrExprAndPlan (void *exprnode, void *plan);
  void *GetCurrExprNode (void);
  void *GetCurrPlan (void);
  NABoolean IsInitializing (void);
private:
  void *m_queryTree;
  void *m_cascadesPlan;
  void *m_currPlan;
  void *m_currExprNode;
  void *m_memo;
  void *m_taskList;
  void *m_analysis;
  void *m_tdb;
  void *m_fragDir;
  Lng32 m_baseAddr;
  Sqlcmpdbg::CompilationPhase m_phase;
  BOOL m_memoSteppingEnabled;
  BOOL m_sqlciInitializing;
};

#endif // QUERYDATA_H
