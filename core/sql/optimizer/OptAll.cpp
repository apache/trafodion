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

#include "Platform.h"

#define SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define SQLPARSERGLOBALS_FLAGS
#define SQLPARSERGLOBALS_NADEFAULTS

#include "Analyzer.cpp"
#include "AppliedStatMan.cpp"
#include "BindItemExpr.cpp"
#include "BindRelExpr.cpp"
#include "BindRI.cpp"
#include "BindWA.cpp"
#include "BinderUtils.cpp"
#include "CacheWA.cpp"
#include "CascadesBasic.cpp"
#include "ChangesTable.cpp"
#include "ColStatDesc.cpp"
#include "ColumnDesc.cpp"
#include "ControlDB.cpp"
#include "Cost.cpp"
#include "costmethod.cpp"
#include "CostScalar.cpp"
#include "DomainDesc.cpp"
#include "EncodedValue.cpp"
#include "EstLogProp.cpp"
#include "GroupAttr.cpp"
#include "ImplRule.cpp"
#include "IndexDesc.cpp"
#include "ItemCache.cpp"
#include "ItemExpr.cpp"
#include "ItemExprList.cpp"
#include "ItemSample.cpp"
#include "ItmBitMuxFunction.cpp"
#include "ItmFlowControlFunction.cpp"
#include "LargeScopeRules.cpp"
//#include "MJVIndexBuilder.cpp"
//#include "MultiJoin.cpp"
//#include "MVInfo.cpp"
//#include "MVJoinGraph.cpp"
//#include "MavRelRootBuilder.cpp"
//#include "MvLog.cpp"
//#include "MvMultiTxnMavBuilder.cpp"
//#include "MvRefreshBuilder.cpp"
//#include "MjvBuilder.cpp"
//#include "mdam.cpp"
//#include "memo.cpp"
//includes below are in optall1.cpp
//#include "NAClusterInfo.cpp"
//#include "NAColumn.cpp"
//#include "NAFileSet.cpp"
//#include "NARoutine.cpp"
//#include "NATable.cpp"
//#include "NodeMap.cpp"
//#include "NormItemExpr.cpp"
//#include "NormRelExpr.cpp"
//#include "NormWA.cpp"

// files below are commented out and are in optall1.cpp

//#include "ObjectNames.cpp"
////#include "opt.cpp"
//#include "OptItemExpr.cpp"
//#include "OptLogRelExpr.cpp"
//#include "OptPhysRelExpr.cpp"
//#include "PackedColDesc.cpp"
//#include "PartFunc.cpp"
//#include "PartKeyDist.cpp"
//#include "PartReq.cpp"
//#include "PhyProp.cpp"
//#include "Refresh.cpp"
//#include "Rel3GL.cpp"
//#include "RelCache.cpp"
//#include "RelDCL.cpp"
//#include "RelExpr.cpp"
//#include "RelPackedRows.cpp"
//#include "RelSample.cpp"
//#include "RelSequence.cpp"
//#include "RelStoredProc.cpp"
//#include "ReqGen.cpp"
//#include "RETDesc.cpp"
//#include "Rule.cpp"
//#include "ScanOptimizer.cpp"
//#include "SchemaDB.cpp"
//#include "ScmCostMethod.cpp"
//#include "SearchKey.cpp"
//#include "SimpleScanOptimizer.cpp"
//#include "Stats.cpp"
//#include "SynthType.cpp"
//#include "TableDesc.cpp"
//#include "TableNameMap.cpp"
//#include "tasks.cpp"
//// need reordering
//#include "AccessSets.cpp"
//#include "TriggerDB.cpp"  
//#include "Triggers.cpp"  
//#include "OptTrigger.cpp"  
//#include "InliningInfo.cpp"
#include "OptHints.cpp"
