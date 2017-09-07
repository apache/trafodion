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
#include "QueryData.h"
 QueryData::QueryData() 
{
}

void QueryData::SetData(void *tree, void *plan) 
{
  void *oldQueryTree = m_queryTree;
  m_queryTree = tree;
  m_cascadesPlan = plan;
  if ((m_queryTree == NULL) && (m_cascadesPlan != NULL))
  {
      
     //--------------------------------------------------------------------------
     // We are in display after phase 1/phase 2 optimization. Hence
     // plan->getPhysicalExpr will provide us the m_queryTree we need for display.
     //--------------------------------------------------------------------------
      CascadesPlan * cPlan = (CascadesPlan *) plan;
      m_queryTree = cPlan->getPhysicalExpr();
  }
  m_sqlciInitializing = FALSE;
  
  //update m_currExprNode
  if (oldQueryTree != m_queryTree)
    if (m_currExprNode == oldQueryTree)
      m_currExprNode = m_queryTree;
    else
      m_currExprNode = NULL;
}

NABoolean QueryData::IsInitializing(void) 
{
  return m_sqlciInitializing;
}

void QueryData::GetData(void **tree, void **plan) 
{
  *tree = m_queryTree;
  *plan = m_cascadesPlan;
}
 
void QueryData::ResetData(void) 
{
  m_cascadesPlan = NULL;
  m_taskList = NULL;
  m_queryTree = NULL;
  m_currExprNode = NULL;
  m_memo = NULL;
  m_analysis = NULL;
  m_tdb = NULL;
  m_fragDir = NULL;
  m_memoSteppingEnabled = FALSE;
  m_baseAddr = 0;
}

void QueryData::SetMemoSteppingFlag(BOOL flag) 
{
  
      //--------------------------------------------------------------------
      // Memo Stepping is enabled only if breakpoint set for display
      // after Normalization phase or after phase 1 optimization.
      //--------------------------------------------------------------------
      m_memoSteppingEnabled = flag;
} 
void QueryData::SetPhase(Sqlcmpdbg::CompilationPhase phase) 
{
  m_phase = phase;
}
Sqlcmpdbg::CompilationPhase QueryData::GetPhase()
{
  return m_phase;
} 
void *QueryData::GetMemo() 
{
  return m_memo;
}

void QueryData::SetMemo(void *memoptr) 
{
  m_memo = memoptr;
} 
void QueryData::SetTaskList(void *tasklist) 
{
  m_taskList = tasklist;
} 
void *QueryData::GetTaskList() 
{
  return m_taskList;
}


void QueryData::SetAnalysis(void *analysis) 
{
  m_analysis = analysis;
  //((QueryAnalysis *) m_analysis)->setInstance((QueryAnalysis *) m_analysis);
}

 
void *QueryData::GetAnalysis() 
{
  return m_analysis;
}

void QueryData::SetTDBData(void *tdb, void *fragDir) 
{
  m_tdb = tdb;
  union {
     struct {
        unsigned int low;
        unsigned int high;
     } data;
     void* ptr;
  } tmp;
  tmp.ptr = tdb;
    m_baseAddr = tmp.data.low;
  m_fragDir = fragDir;
} 
void QueryData::GetTDBData(void **tdb, void **fragDir, Lng32 & baseAddr) 
{
  *tdb = m_tdb;
  *fragDir = m_fragDir;
  baseAddr = m_baseAddr;
} 
void QueryData::SetCurrExprAndPlan(void *exprnode, void *plan) 
{
  m_currPlan = plan;
  m_currExprNode = exprnode;
} 
void *QueryData::GetCurrExprNode() 
{
  return m_currExprNode;
}

void *QueryData::GetCurrPlan() 
{
  return m_currPlan;
}


