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
/* -*-C++-*-
******************************************************************************
*
* File:         ImplRule.C
* Description:  DBI-defined implementation rules
* Created:      9/14/94
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "Sqlcomp.h"
#include "GroupAttr.h"
#include "opt.h"
#include "PhyProp.h"
#include "ImplRule.h"
#include "AllRelExpr.h"
#include "RelPackedRows.h"
#include "RelSequence.h"
#include "RelSample.h"
#include "AllItemExpr.h"
#include "ValueDesc.h"
#include "mdam.h"
#include "CmpContext.h"
#include "Cost.h"
#include "CostMethod.h"
#include "ScanOptimizer.h"

#include "NADefaults.h"
#include "OptimizerSimulator.h"
#include "RelScan.h"

// -----------------------------------------------------------------------
// Global variables
// -----------------------------------------------------------------------
THREAD_P NAUnsigned              HashJoinRuleNumber;
THREAD_P NAUnsigned              NestedJoinRuleNumber;
THREAD_P NAUnsigned              MergeJoinRuleNumber;
THREAD_P NAUnsigned              SortEnforcerRuleNumber;

// -----------------------------------------------------------------------
// Global function to add implementation rules to the rule set
// -----------------------------------------------------------------------
static const NAString LiteralAnyName("any name");

void CreateImplementationRules(RuleSet* set)
{
  Rule *r;

  CorrName anyCorrName(LiteralAnyName);

  r = new(CmpCommon::contextHeap()) FileScanRule
    ("File Scan implementation rule",
     new(CmpCommon::contextHeap()) Scan(REL_SCAN, CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
       FileScan(CorrName(),
                NULL,
                NULL,
                REL_FILE_SCAN,
                CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  r = new(CmpCommon::contextHeap()) HbaseScanRule
    ("Hbase Scan implementation rule",
     new(CmpCommon::contextHeap()) Scan(REL_SCAN, CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
     HbaseAccess(REL_HBASE_ACCESS, CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  r = new(CmpCommon::contextHeap()) HashJoinRule
    ("Hash join implementation rule",
     new(CmpCommon::contextHeap())
       WildCardOp(REL_ANY_JOIN,
                  0,
                  new(CmpCommon::contextHeap())
                    CutOp(0, CmpCommon::contextHeap()),
                  new(CmpCommon::contextHeap())
                    CutOp(1, CmpCommon::contextHeap()),
                  CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
     HashJoin(new(CmpCommon::contextHeap())
                CutOp(0, CmpCommon::contextHeap()),
              new(CmpCommon::contextHeap())
                CutOp(1, CmpCommon::contextHeap()),
              CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  HashJoinRuleNumber = r->getNumber();

  r = new(CmpCommon::contextHeap())
    MergeJoinRule(
                  "Merge join implementation rule",
                  new(CmpCommon::contextHeap())
                  WildCardOp(REL_ANY_JOIN,
                             0,
                             new(CmpCommon::contextHeap())
                               CutOp(0, CmpCommon::contextHeap()),
                             new(CmpCommon::contextHeap())
                               CutOp(1, CmpCommon::contextHeap()),
                             CmpCommon::contextHeap()),
                  new(CmpCommon::contextHeap())
                  MergeJoin(new(CmpCommon::contextHeap())
                              CutOp(0, CmpCommon::contextHeap()),
                            new(CmpCommon::contextHeap())
                              CutOp(1, CmpCommon::contextHeap()),
                            REL_MERGE_JOIN,
                            NULL,
                            CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber(),
                        set->getSecondPassNumber());

  // set global variable
  MergeJoinRuleNumber = r->getNumber();

  r = new(CmpCommon::contextHeap()) NestedJoinRule
    ("Nested join implementation rule",
     new(CmpCommon::contextHeap())
     WildCardOp(REL_ANY_JOIN,
                0,
                new(CmpCommon::contextHeap())
                  CutOp(0, CmpCommon::contextHeap()),
                new(CmpCommon::contextHeap())
                  CutOp(1, CmpCommon::contextHeap()),
                CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
     NestedJoin(new(CmpCommon::contextHeap())
                  CutOp(0, CmpCommon::contextHeap()),
                new(CmpCommon::contextHeap())
                  CutOp(1, CmpCommon::contextHeap()),
                REL_NESTED_JOIN,
                CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  NestedJoinRuleNumber = r->getNumber();

  r = new(CmpCommon::contextHeap()) NestedJoinFlowRule
    ("Nested join flow implementation rule",
     new(CmpCommon::contextHeap())
     Join(new(CmpCommon::contextHeap())
            CutOp(0, CmpCommon::contextHeap()),
          new(CmpCommon::contextHeap())
            CutOp(1, CmpCommon::contextHeap()),
          REL_TSJ_FLOW,
          NULL,
          FALSE,
          FALSE,
          CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
     NestedJoinFlow(new(CmpCommon::contextHeap())
                      CutOp(0, CmpCommon::contextHeap()),
                    new(CmpCommon::contextHeap())
                      CutOp(1, CmpCommon::contextHeap()),
                    NULL,
                    NULL,
                    CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  r = new(CmpCommon::contextHeap()) HashGroupByRule
    ("Impl: Groupby with a hash table",
     new(CmpCommon::contextHeap())
     WildCardOp(REL_ANY_GROUP,
                0,
                new(CmpCommon::contextHeap())
                  CutOp(0, CmpCommon::contextHeap()),
                NULL,
                CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
     HashGroupBy(new(CmpCommon::contextHeap())
                   CutOp(0, CmpCommon::contextHeap()),
                 REL_HASHED_GROUPBY,
                 NULL,
                 NULL,
                 CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  r = new(CmpCommon::contextHeap()) SortGroupByRule
    ("Impl: Groupby of sorted table",
     new(CmpCommon::contextHeap())
     WildCardOp(REL_ANY_GROUP,
                0,
                new(CmpCommon::contextHeap())
                  CutOp(0, CmpCommon::contextHeap()),
                NULL,
                CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
     SortGroupBy(new(CmpCommon::contextHeap())
                   CutOp(0, CmpCommon::contextHeap()),
                 REL_ORDERED_GROUPBY,
                 NULL,
                 NULL,
                 CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber(),
                        set->getSecondPassNumber());

  r = new(CmpCommon::contextHeap()) AggregateRule
    ("Impl: Aggregate with no grouping columns",
     new(CmpCommon::contextHeap())
     WildCardOp(REL_ANY_GROUP,
                0,
                new(CmpCommon::contextHeap())
                  CutOp(0, CmpCommon::contextHeap()),
                NULL,
                CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
     SortGroupBy(new(CmpCommon::contextHeap())
                   CutOp(0, CmpCommon::contextHeap()),
                 REL_ORDERED_GROUPBY,
                 NULL,
                 NULL,
                 CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  r = new(CmpCommon::contextHeap()) PhysShortCutGroupByRule
    ("Implement ShortCutGroupBy by a PhysicalShortCutGroupBy ",
     new(CmpCommon::contextHeap())
       ShortCutGroupBy(new(CmpCommon::contextHeap())
                        CutOp(0, CmpCommon::contextHeap()),
                       REL_SHORTCUT_GROUPBY,
                       NULL,
                       NULL,
                       CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
       PhysShortCutGroupBy(new(CmpCommon::contextHeap())
                            CutOp(0, CmpCommon::contextHeap()),
                           REL_SHORTCUT_GROUPBY,
                           NULL,
                           NULL,
                           CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber(),
                        set->getSecondPassNumber());

  r = new(CmpCommon::contextHeap()) UnionRule
    ("Implementation rule for Union nodes",
     new(CmpCommon::contextHeap())
       Union(new(CmpCommon::contextHeap()) CutOp(0, CmpCommon::contextHeap()),
             new(CmpCommon::contextHeap()) CutOp(1, CmpCommon::contextHeap()),
             NULL,
             NULL,
             REL_UNION,
             CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
       MergeUnion(new(CmpCommon::contextHeap())
                    CutOp(0, CmpCommon::contextHeap()),
                  new(CmpCommon::contextHeap())
                    CutOp(1, CmpCommon::contextHeap()),
                  NULL,
                  REL_MERGE_UNION,
                  CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  r = new(CmpCommon::contextHeap()) HbaseDeleteRule
    ("Hbase Delete Implementation rule",
     new(CmpCommon::contextHeap())
       Delete(anyCorrName,
              NULL,
              REL_UNARY_DELETE,
              new(CmpCommon::contextHeap())
                Scan(REL_SCAN, CmpCommon::contextHeap()),
              NULL, NULL,
	      NULL,
              CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
       HbaseDelete(
                 CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  r = new(CmpCommon::contextHeap()) HbaseUpdateRule
    ("Hbase Update Implementation rule",
     new(CmpCommon::contextHeap())
       Update(anyCorrName,
              NULL,
              REL_UNARY_UPDATE,
              new(CmpCommon::contextHeap())
                Scan(REL_SCAN, CmpCommon::contextHeap()),
              NULL, NULL,
              CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
       HbaseUpdate(
                 CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  r = new(CmpCommon::contextHeap()) HiveInsertRule
    ("Implementation rule for hive Insert",
     new(CmpCommon::contextHeap())
       Insert(anyCorrName,
              NULL,
              REL_LEAF_INSERT,
              NULL,
              NULL,
              NULL,
              CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
     HiveInsert(anyCorrName,
                        NULL,
                        REL_HIVE_INSERT,
                        NULL,
               CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  r = new(CmpCommon::contextHeap()) HbaseInsertRule
    ("Implementation rule for hbase Insert",
     new(CmpCommon::contextHeap())
       Insert(anyCorrName,
              NULL,
              REL_LEAF_INSERT,
              NULL,
              NULL,
              NULL,
              CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
     HbaseInsert(anyCorrName,
                        NULL,
                        REL_HBASE_INSERT,
                        NULL,
               CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  r = new(CmpCommon::contextHeap()) HbaseDeleteCursorRule
    ("Hbase Delete using cursor",
     new(CmpCommon::contextHeap())
       Delete(anyCorrName,
              NULL,
              REL_LEAF_DELETE,
	      NULL,
              NULL, NULL,
	      NULL,
              CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
       HbaseDelete(
                 CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  r = new(CmpCommon::contextHeap()) HbaseUpdateCursorRule
    ("Hbase Update using cursor",
     new(CmpCommon::contextHeap())
       Update(anyCorrName,
              NULL,
              REL_LEAF_UPDATE,
	      NULL,
              NULL, NULL,
              CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
       HbaseUpdate(
                 CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  r = new(CmpCommon::contextHeap()) PhysicalMapValueIdsRule
    ("Implement MapValueIds by a PhysicalMapValueIds",
     new(CmpCommon::contextHeap())
     MapValueIds(new(CmpCommon::contextHeap())
                   CutOp(0, CmpCommon::contextHeap()),
                 CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
     PhysicalMapValueIds(new(CmpCommon::contextHeap())
                           CutOp(0, CmpCommon::contextHeap()),
                         CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  r = new(CmpCommon::contextHeap()) PhysicalRelRootRule
    ("Implement RelRoot by a PhysicalRelRoot",
     new(CmpCommon::contextHeap())
     RelRoot(new(CmpCommon::contextHeap())
               CutOp(0, CmpCommon::contextHeap()),
             REL_ROOT,
             NULL,
             NULL,
             NULL,
             NULL,
             CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
     PhysicalRelRoot(new(CmpCommon::contextHeap())
                       CutOp(0, CmpCommon::contextHeap()),
                     CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  r = new(CmpCommon::contextHeap()) PhysicalTupleRule
    ("Implement Tuple by a PhysicalTuple",
     new(CmpCommon::contextHeap()) Tuple(NULL, CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap()) PhysicalTuple(CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  r = new(CmpCommon::contextHeap()) PhysicalTupleListRule
   ("Implement TupleList by a PhysicalTupleList",
    new(CmpCommon::contextHeap()) TupleList(NULL, CmpCommon::contextHeap()),
    new(CmpCommon::contextHeap()) PhysicalTupleList(CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  r = new(CmpCommon::contextHeap()) SortEnforcerRule(
       "Enforcer rule to sort a result",
       new(CmpCommon::contextHeap())
         CutOp(0, CmpCommon::contextHeap()),
       new(CmpCommon::contextHeap())
         Sort(new(CmpCommon::contextHeap())
                CutOp(0, CmpCommon::contextHeap()),
              CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  // set global variable
  SortEnforcerRuleNumber = r->getNumber();

  r = new(CmpCommon::contextHeap()) ExchangeEnforcerRule(
       "Enforce partitioning or plan location",
       new(CmpCommon::contextHeap())
         CutOp(0, CmpCommon::contextHeap()),
       new(CmpCommon::contextHeap())
         Exchange(new(CmpCommon::contextHeap())
                    CutOp(0, CmpCommon::contextHeap()),
                  CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  r = new(CmpCommon::contextHeap()) PhysicalExplainRule
    ("Implement ExplainFunc by a PhysicalExplain",
     new(CmpCommon::contextHeap())
       ExplainFunc(NULL, CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
       PhysicalExplain(CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  r = new(CmpCommon::contextHeap()) PhysicalHiveMDRule
    ("Implement HiveMDaccessFunc by a PhysicalExplain",
     new(CmpCommon::contextHeap())
     HiveMDaccessFunc(NULL, NULL, NULL, CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
       PhysicalHiveMD(CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  r = new(CmpCommon::contextHeap()) PhysicalTransposeRule
    ("Implement Transpose by a PhysicalTranspose",
     new(CmpCommon::contextHeap())
     Transpose(NULL,
               NULL,
               new(CmpCommon::contextHeap())
               CutOp(0, CmpCommon::contextHeap()),
               CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
     PhysTranspose(new(CmpCommon::contextHeap())
                   CutOp(0, CmpCommon::contextHeap()),
                   CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  r = new(CmpCommon::contextHeap()) PhysicalPackRule
    ("Implement Pack by a PhysicalPack",
     new(CmpCommon::contextHeap())
     Pack(0,
          new(CmpCommon::contextHeap()) CutOp(0,CmpCommon::contextHeap()),
          NULL,
          CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
     PhyPack(0,
             new(CmpCommon::contextHeap()) CutOp(0,CmpCommon::contextHeap()),
             CmpCommon::contextHeap())
     );
  set->insert(r);
  set->enable(r->getNumber());

  r = new(CmpCommon::contextHeap()) PhysicalUnPackRowsRule
    ("Implement UnPackRows by a PhysUnPackRows",
     new(CmpCommon::contextHeap())
     UnPackRows(0,
                NULL,
                NULL,
                NULL,
                new(CmpCommon::contextHeap())
                CutOp(0,CmpCommon::contextHeap()),
                NULL_VALUE_ID,
                CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
     PhysUnPackRows(new(CmpCommon::contextHeap())
                    CutOp(0,CmpCommon::contextHeap()),
                    CmpCommon::contextHeap())
     );
  set->insert(r);
  set->enable(r->getNumber());

  r = new (CmpCommon::contextHeap())
          PhysCompoundStmtRule
                ( "CompoundStmt Operator to PhysCompoundStmt operator",
                   new (CmpCommon::contextHeap())
                       CompoundStmt(
                         new (CmpCommon::contextHeap())
                           CutOp(0, CmpCommon::contextHeap()),
                         new (CmpCommon::contextHeap())
                           CutOp(1, CmpCommon::contextHeap()),
                         REL_COMPOUND_STMT, CmpCommon::contextHeap()),
                   new (CmpCommon::contextHeap())
                       PhysCompoundStmt(new (CmpCommon::contextHeap())
                         CutOp(0, CmpCommon::contextHeap()),
                           new (CmpCommon::contextHeap())
                         CutOp(1, CmpCommon::contextHeap()),
                         REL_COMPOUND_STMT,
                         CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  r = new(CmpCommon::contextHeap()) PhysicalSequenceRule
    ("Implement RelSequence by a PhysSequence",
     new(CmpCommon::contextHeap())
     RelSequence(new(CmpCommon::contextHeap())
                 CutOp(0,CmpCommon::contextHeap()),
                 NULL,
                 CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
     PhysSequence(new(CmpCommon::contextHeap())
                  CutOp(0,CmpCommon::contextHeap()),
                  CmpCommon::contextHeap())
     );
  set->insert(r);
  set->enable(r->getNumber());

  r = new(CmpCommon::contextHeap()) PhysicalSampleRule
    ("Implement RelSample by a PhysSample",
     new(CmpCommon::contextHeap())
     RelSample(new(CmpCommon::contextHeap())
               CutOp(0,CmpCommon::contextHeap()),
               RelSample::ANY,
               NULL,
               NULL,
               CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
     PhysSample(new(CmpCommon::contextHeap())
                CutOp(0,CmpCommon::contextHeap()),
                RelSample::ANY,
                CmpCommon::contextHeap())
     );
  set->insert(r);
  set->enable(r->getNumber());

  r = new (CmpCommon::contextHeap()) PhysicalSPProxyFuncRule
    ("Implement SPProxyFunc by a PhysicalSPProxyFunc",
     new (CmpCommon::contextHeap())
       SPProxyFunc(CmpCommon::contextHeap()),
     new (CmpCommon::contextHeap())
       PhysicalSPProxyFunc(CmpCommon::contextHeap())
     );
  set->insert(r);
  set->enable(r->getNumber());

  r = new (CmpCommon::contextHeap()) PhysicalExtractSourceRule
    ("Implement ExtractSource by a PhysicalExtractSource",
     new (CmpCommon::contextHeap())
       ExtractSource(CmpCommon::contextHeap()),
     new (CmpCommon::contextHeap())
       PhysicalExtractSource(CmpCommon::contextHeap())
     );
  set->insert(r);
  set->enable(r->getNumber());

  r = new(CmpCommon::contextHeap()) PhysicalIsolatedScalarUDFRule
    ("Implement IsolatedScalarUDF by a PhysicalIsolatedScalarUDF",
     new(CmpCommon::contextHeap())
     IsolatedScalarUDF(NULL, CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
                   PhysicalIsolatedScalarUDF(CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  r = new (CmpCommon::contextHeap())
          PhysicalTMUDFRule
             ("Implement a Table Mapping Function",
              NULL,
              NULL
              );
  set->insert(r);
  set->enable(r->getNumber());

  r = new (CmpCommon::contextHeap())
            PhysicalFastExtractRule
               ("Implement a Fast Extract",
                new (CmpCommon::contextHeap())
                  FastExtract(new(CmpCommon::contextHeap())
                                CutOp(0,CmpCommon::contextHeap()),
                              CmpCommon::contextHeap()),
                new (CmpCommon::contextHeap())
                  PhysicalFastExtract(new(CmpCommon::contextHeap())
                                        CutOp(0,CmpCommon::contextHeap()),
                                      CmpCommon::contextHeap())
                );
    set->insert(r);
    set->enable(r->getNumber());
}

// -----------------------------------------------------------------------
// Static methods for rules operating on GenericUpdate
// (should be a GU method but I'm too lazy to do all the recompiles entailed)
// -----------------------------------------------------------------------

void copyCommonGenericUpdateFields(GenericUpdate *result,
                                   /*const*/ GenericUpdate *bef,
                                   NABoolean setSelectStoi = FALSE)
{
  result->setGroupAttr(bef->getGroupAttr());

  result->updateToSelectMap()      = bef->updateToSelectMap();
  result->newRecExpr()             = bef->newRecExpr();
  result->newRecExprArray()        = bef->newRecExprArray();
  // QSTUFF
  result->newRecBeforeExpr()       = bef->newRecBeforeExpr();
  result->newRecBeforeExprArray()  = bef->newRecBeforeExprArray();
  // QSTUFF
  result->accessOptions()          = bef->accessOptions();
  result->checkConstraints()       = bef->checkConstraints();

  result->beginKeyPred()           = bef->beginKeyPred();
  result->endKeyPred()             = bef->endKeyPred();
  result->executorPred()           = bef->executorPred();
  result->indexNewRecExprArrays()  = bef->indexNewRecExprArrays();
  result->indexBeginKeyPredArray() = bef->indexBeginKeyPredArray();
  result->indexEndKeyPredArray()   = bef->indexEndKeyPredArray();
  result->indexNumberArray()       = bef->indexNumberArray();

  result->setIndexDesc(bef->getTableDesc()->getClusteringIndex());
  result->setScanIndexDesc(bef->getScanIndexDesc());

  result->setMtsStatement(bef->isMtsStatement());

  result->setIsMergeUpdate(bef->isMergeUpdate());
  result->setIsMergeDelete(bef->isMergeDelete());

  result->setNoRollbackOperation(bef->isNoRollback());

  // potentialOutputs_ needs to be copied, even in an implementation rule,
  // for the DP2xxx::codeGen - getReturnRow - producesOutputs call.
  ValueIdSet outputs;
  bef->getPotentialOutputValues(outputs);
  result->setPotentialOutputValues(outputs);

  result->setOptStoi(bef->getOptStoi());
  if (setSelectStoi)
    result->getOptStoi()->getStoi()->setSelectAccess();

  if (bef->currOfCursorName())
    result->currOfCursorName() =
      bef->currOfCursorName()->copyTree(CmpCommon::statementHeap())->castToItemExpr();

  // Note that these last are not simple *copying* operations...
  result->computeUsedCols();
  result->finalizeRowsAffected(bef);

  result->getInliningInfo().merge(&bef->getInliningInfo());

  result->setAvoidHalloweenR2(bef->avoidHalloweenR2());
  result->setAvoidHalloween(bef->avoidHalloween());
  result->setHalloweenCannotUseDP2Locks(bef->getHalloweenCannotUseDP2Locks());
  result->setUpdateCKorUniqueIndexKey(bef->getUpdateCKorUniqueIndexKey());

  // this field is in the base class RelExpr.
  // We should write a RelExpr::copyCommonRelExprFields method instead
  // of copying them here.
  result->setTolerateNonFatalError(bef->getTolerateNonFatalError());
  result->setNoCheck(bef->noCheck());
  result->setPrecondition(bef->getPrecondition());
  result->setProducedMergeIUDIndicator(bef->getProducedMergeIUDIndicator());
  result->setReferencedMergeIUDIndicator(bef->getReferencedMergeIUDIndicator());
}

void copyCommonUpdateFields(Update *result,
			    /*const*/ Update *bef)
{
  result->setGroupAttr(bef->getGroupAttr());

  result->mergeInsertRecExpr()          = bef->mergeInsertRecExpr();
  result->mergeInsertRecExprArray()     = bef->mergeInsertRecExprArray();
  result->mergeUpdatePred()             = bef->mergeUpdatePred();
  
}

void copyCommonDeleteFields(Delete *result,
			    /*const*/ Delete *bef)
{
  result->setGroupAttr(bef->getGroupAttr());

  result->mergeInsertRecExpr()          = bef->mergeInsertRecExpr();
  result->mergeInsertRecExprArray()     = bef->mergeInsertRecExprArray();

  result->csl() = bef->csl();
}

// -----------------------------------------------------------------------
// methods for class FileScanRule
// -----------------------------------------------------------------------

FileScanRule::~FileScanRule() {} 

/**************************************************************************
* Input : list of indexes of the same table
* Output: smallest index and position of the index in the list
* Finds smallest index based on Kb per volume which can be normalized to
* rowSize/volumes because rowCount for the indexes are same.
***************************************************************************/


IndexDesc * findSmallestIndex(const LIST(IndexProperty *) & indexes /*in*/,
			      CollIndex& entry/*out*/)
{
  CostScalar minKbPerVol=csZero;
  CostScalar kbPerVol=csZero;
  IndexDesc * smallestIndex = NULL;
  IndexDesc * index = NULL;
  CollIndex numIndexOnlyIndexes = indexes.entries();
  for(CollIndex i=0;i<numIndexOnlyIndexes;i++)
    {
      index = indexes[i]->getIndexDesc();
      if (index->isClusteringIndex())
	{
	  const PartitioningFunction* physicalPartFunc =
	    index->getPartitioningFunction();

	  // if an explicit partition range has been specified 
	  // as part of the table name to restrict the scan on user specified
	  // partitions, then disable index scan.
	  // This is needed since the index and base table partitions may
	  // not be distributed the same way and an index only scan may not
	  // restrict the partitions as intended by user.
	  // Return the clustering index
	  if (physicalPartFunc && 
	      physicalPartFunc->partitionRangeRestricted())
	    {
	      entry = i;
	      smallestIndex = index;
	      break;
	    }
	}

      kbPerVol = index->getKbPerVolume();
      if(kbPerVol < minKbPerVol OR smallestIndex == NULL)
      {
	minKbPerVol = kbPerVol;
	smallestIndex = index;
	entry = i;
      }
    }
  return smallestIndex;

}

/****************************************************************
* Input : set of indexes of the same table
* Output: smallest index
* Finds smallest index based on Kb per volume which can be normalized to
* rowSize/volumes because rowCount for the indexes are same.
****************************************************************/

IndexDesc * findSmallestIndex(const SET(IndexDesc *) & indexes)
{
  CostScalar minKbPerVol=csZero;
  CostScalar kbPerVol=csZero;
  IndexDesc * smallestIndex = NULL;
  IndexDesc * index = NULL;
  CollIndex numIndexOnlyIndexes = indexes.entries();
  int maxPriorityDelta = 0;
  int priorityDelta = 0;

  for(CollIndex i=0;i<numIndexOnlyIndexes;i++)
    {
      index = indexes[i];
      if (index->isClusteringIndex())
	{
	  const PartitioningFunction* physicalPartFunc =
	    index->getPartitioningFunction();

	  // if an explicit partition range has been specified 
	  // as part of the table name to restrict the scan on user specified
	  // partitions, then disable index scan.
	  // This is needed since the index and base table partitions may
	  // not be distributed the same way and an index only scan may not
	  // restrict the partitions as intended by user.
	  // Return the clustering index
	  if (physicalPartFunc && 
	      physicalPartFunc->partitionRangeRestricted())
	    {
	      smallestIndex = index;
	      break;
	    }
	}

      priorityDelta = index->indexHintPriorityDelta();
      // check priority first, then size
      if (priorityDelta >= maxPriorityDelta)
      {
        kbPerVol = index->getKbPerVolume();
        if(kbPerVol < minKbPerVol OR
           smallestIndex == NULL OR
           priorityDelta > maxPriorityDelta)
          {
            minKbPerVol = kbPerVol;
            smallestIndex = index;
          }
        maxPriorityDelta = priorityDelta;
      }
    }
  return smallestIndex;

}

/**************************************************************************
* Input : list of indexes
* Output: Most partitioned index and position of the index in the list
***************************************************************************/

IndexDesc * findMostPartitionedIndex(const LIST(IndexProperty *)& indexes,CollIndex& entry)
{
  CollIndex numPartitions = 0;
  CollIndex maxNumPartitions = 0;
  IndexDesc * index = NULL;
  IndexDesc * mostPartitionedIndex = NULL;
  CollIndex numIndexOnlyIndex = indexes.entries();
  for(CollIndex i=0;i<numIndexOnlyIndex;i++)
    {
      index = indexes[i]->getIndexDesc();
      numPartitions = (index->getPartitioningFunction()?((NodeMap *)(index->
	getPartitioningFunction()->getNodeMap()))->getNumActivePartitions():1);
      if(numPartitions > maxNumPartitions OR mostPartitionedIndex == NULL)
      {
	maxNumPartitions = numPartitions;
	mostPartitionedIndex = index;
	entry = i;
      }
    }
  return mostPartitionedIndex;
}
/**************************************************************************
* Input : list of indexes
* Output: Return TRUE if one of the index can provide a promising index join
* plan.
***************************************************************************/
// This function is never code in the code base (checked in M5):1064
NABoolean oneViableIndexJoin(const LIST(IndexProperty *) & indexes)
{
  CollIndex ixCount = indexes.entries();
  for(CollIndex i=0;i<ixCount; i++)
  {
    if(indexes[i]->getSelectivity() == INDEX_JOIN_VIABLE)
    {
      return TRUE;
    }
  }
  return FALSE;
}


/**************************************************************************
* Input : list of indexes, order comparison enums and input context.
* Output: Removes indexes from the list that are not promising for an index
* join plan. Removes corresponding oc enums. Have to make sure inputEstLogProp
* is in the error range of initialEstLogProp used to determine the promise.
***************************************************************************/
void removeLowSelectivityIndexJoins(	LIST(IndexProperty *)& indexes,
					LIST(OrderComparison)& ocEnums,
					const Context * context
					)
{

  CostScalar inputCardinality = csOne;
  CostScalar defInputCardinality = csOne;
  if(context->getInputLogProp())
    inputCardinality = context->getInputLogProp()->getResultCardinality();
  if(indexes[0]->getInputEstLogProp())
    defInputCardinality = indexes[0]->getInputEstLogProp()->getResultCardinality();
  if(inputCardinality > defInputCardinality * CURRSTMT_OPTDEFAULTS->acceptableInputEstLogPropError())
  {
    for(CollIndex i=0;i<indexes.entries();i++)
    {
      if(indexes[i]->getSelectivity()==EXCEEDS_BT_SCAN AND indexes.entries() >1)
      {
	indexes.removeAt(i);
	ocEnums.removeAt(i);
	i--;
      }
    }
  }
}

/**************************************************************************
* Input : list of indexes
* Output: Return TRUE if one of the index has bad key access or in other words
* MDAM is not viable.
***************************************************************************/
NABoolean oneWithMdamOff(const LIST(IndexProperty *) & indexes)
{
  CollIndex ixCount = indexes.entries();
  for(CollIndex i=0;i<ixCount; i++)
  {
    if(indexes[i]->getMdamFlag() == MDAM_OFF)
    {
      return TRUE;
    }
  }
  return FALSE;
}

/**************************************************************************
* Input : list of indexes
* Output: Return TRUE if one of the index has good key access or in other
* words MDAM is viable.
***************************************************************************/
NABoolean oneWithMdamOn(const LIST(IndexProperty *) & indexes)
{
  CollIndex ixCount = indexes.entries();
  for(CollIndex i=0;i<ixCount; i++)
  {
    if(indexes[i]->getMdamFlag() == MDAM_ON)
    {
      return TRUE;
    }
  }
  return FALSE;
}

/*************************************************************************
* Input: Index list and corresponding OrderComparisons.
* Output: Removes indexes that have bad key access unless they are partitioned
* better than the indexes with good key access.
*************************************************************************/
void removeBadKeyIndexes( LIST(IndexProperty *)& indexes,
			  LIST(OrderComparison)& ocEnums
			  )
{
  CollIndex numOfVols = 1;
  CollIndex maxNumOfVols = 1;
  if(oneWithMdamOff(indexes) AND oneWithMdamOn(indexes))
  {
    for(CollIndex i=0;i<indexes.entries();i++)
    {
      if(indexes[i]->getMdamFlag() == MDAM_ON)
      {
	numOfVols = (indexes[i]->getIndexDesc()->getPartitioningFunction()?((NodeMap *)(indexes[i]
		->getIndexDesc()->getPartitioningFunction()->getNodeMap()))->getNumOfDP2Volumes():1);
	if(numOfVols > maxNumOfVols)
	{
	  maxNumOfVols = numOfVols;
	}
      }
    }

    if(maxNumOfVols >1)
    {
      for(CollIndex j=0;j<indexes.entries();j++)
      {
       if(indexes[j]->getMdamFlag() == MDAM_OFF AND (indexes[j]->getIndexDesc()->getPartitioningFunction() == NULL
	OR ((NodeMap *)(indexes[j]->getIndexDesc()->getPartitioningFunction()
		->getNodeMap()))->getNumOfDP2Volumes() <= maxNumOfVols))
	{
	  indexes.removeAt(j);
	  ocEnums.removeAt(j);
	  j--;
	}
      }
    }
  }
}

/*********************************************************************
* Just a cut and pasted from FileScanRule::nextSubstitute(). Creates
* DP2Scan for the given index.
*********************************************************************/
void createAndInsertDP2Scan( const IndexDesc * idesc,
			     Scan * bef,
			      RuleSubstituteMemory *& memory,
			      const Disjuncts * disjunctsPtr,
			      OrderComparison oc,
                             MdamFlags ixMdamFlag = UNDECIDED)
{
  // generate a file scan node to scan the index

        FileScan *fileScan =
            new(CmpCommon::statementHeap())
          DP2Scan(bef->getTableName(),
                  bef->getTableDesc(),
                  idesc,
                  oc==INVERSE_ORDER,
                  bef->getBaseCardinality(),
                  bef->accessOptions(),
                  bef->getGroupAttr(),
                  bef->getSelectionPred(),
                  *disjunctsPtr);


        (void) bef->copyTopNode(fileScan, CmpCommon::statementHeap());

        fileScan->setOptStoi(bef->getOptStoi());
        fileScan->setSingleVerticalPartitionScan(
                    bef->isSingleVerticalPartitionScan());
        fileScan->pkeyHvarList() = bef->pkeyHvarList();

        // Set the sampling related fields
        fileScan->sampledColumns() += bef->sampledColumns();
        fileScan->samplePercent(bef->samplePercent());
        fileScan->clusterSize(bef->clusterSize());
	fileScan->setMdamFlag(ixMdamFlag);

        if (fileScan->isHiveTable())
          {
            ValueIdSet preds(bef->selectionPred());
            HivePartitionAndBucketKey *hpk =
              new(CmpCommon::statementHeap()) HivePartitionAndBucketKey(
                   bef->getTableDesc()->getClusteringIndex()->
                   getNAFileSet()->getHHDFSTableStats(),
                   ValueIdList(), // dummies for now
                   ValueIdList(),
                   preds);
            fileScan->setHiveSearchKey(hpk);
          }

        // add the file scan to the list of substitutes
        memory->insert(fileScan);
}

void createAndInsertHbaseScan(IndexDesc * idesc,
			     Scan * bef,
			     RuleSubstituteMemory *& memory,
			     //const MaterialDisjuncts * disjunctsPtr,
			     const Disjuncts * disjunctsPtr,
                             const ValueIdSet &generatedCCPreds,
			     OrderComparison oc,
                             MdamFlags ixMdamFlag = UNDECIDED)
{
  // generate a hbase scan node 

   HbaseAccess * hbaseScan = new(CmpCommon::statementHeap())
          HbaseAccess(bef->getTableName(),
                  bef->getTableDesc(),
                  idesc,
                  oc==INVERSE_ORDER,
                  bef->getBaseCardinality(),
                  bef->accessOptions(),
                  bef->getGroupAttr(),
                  bef->getSelectionPred(),
                  *disjunctsPtr,
                  generatedCCPreds
                 );

   idesc->getPrimaryTableDesc()->getTableColStats();

   (void) bef->copyTopNode(hbaseScan, CmpCommon::statementHeap());

   hbaseScan->setOptStoi(bef->getOptStoi());
   hbaseScan->setSingleVerticalPartitionScan(
               bef->isSingleVerticalPartitionScan());
   hbaseScan->pkeyHvarList() = bef->pkeyHvarList();

   // Set the sampling related fields
   hbaseScan->sampledColumns() += bef->sampledColumns();
   hbaseScan->samplePercent(bef->samplePercent());
   hbaseScan->clusterSize(bef->clusterSize());
   hbaseScan->setMdamFlag(ixMdamFlag);


  /////////////////////////////////////////////////////////////////////////
  // now set the group attributes of the result's top node
  hbaseScan->setGroupAttr(bef->getGroupAttr());

  // add the scan to the list of substitutes
  memory->insert(hbaseScan);
}

void createAndInsertScan(IndexDesc * idesc,
			 Scan * bef,
			 RuleSubstituteMemory *& memory,
			 //const MaterialDisjuncts * disjunctsPtr,
			 const Disjuncts * disjunctsPtr,
                         const ValueIdSet &generatedCCPreds,
			 OrderComparison oc,
                         MdamFlags ixMdamFlag = UNDECIDED,
                         NABoolean isHbase = FALSE)
{
   if ( !isHbase )
     createAndInsertDP2Scan(idesc, bef, memory, disjunctsPtr, oc, ixMdamFlag);
   else
     createAndInsertHbaseScan(idesc, bef, memory, disjunctsPtr, generatedCCPreds, oc, ixMdamFlag);
}

NABoolean FileScanRule::topMatch(RelExpr * relExpr, Context *context)
{
  if (NOT Rule::topMatch(relExpr,context))
    return FALSE;

  const Scan * bef = (Scan *) relExpr;

  if ((bef->isHbaseTable()))
     return FALSE; // hbase scan is handled by the HbaseScanRule


  if ((bef->isHiveTable()))
    {
      const ReqdPhysicalProperty* rppForMe = context->getReqdPhysicalProperty();
      PartitioningRequirement * partReq = rppForMe->getPartitioningRequirement();

      // Hive table scan executes in master or ESP
      if (rppForMe->executeInDP2())
        return FALSE;

      // The Hive scan can handle only fuzzy and single partition requests for now
      if (partReq &&
          ! partReq->isRequirementExactlyOne() &&
          ! partReq->isRequirementFuzzy())
        return FALSE;

      // A hive scan doesn't have a partitioning key for now (change that later)
      //
      if (partReq &&
          partReq->partitioningKeyIsSpecified() &&
          ! partReq->isRequirementExactlyOne())
        return FALSE;
    }
  else
    {
      // Regular DP2Scan can only execute in DP2.
      if (NOT context->getReqdPhysicalProperty()->executeInDP2())
        return FALSE;
    }

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  if (relExpr->rppRequiresEnforcer(context->getReqdPhysicalProperty()))
    return FALSE;

  return TRUE;

}

RelExpr * generateScanSubstitutes(RelExpr * before,
                                  Context * context,
                                  RuleSubstituteMemory *& memory, NABoolean isHbase)
{
  RelExpr *result;

  if (memory == NULL)
  {
    // -----------------------------------------------------------------
    // this is the first call, create all possible file scans
    // -----------------------------------------------------------------

    CMPASSERT(before->getOperatorType() == REL_SCAN);

    // input structures (readonly)
    Scan * bef = (Scan *) before;

    NABoolean resultNeedsToBeOrdered = (context AND context->requiresOrder());
    const ReqdPhysicalProperty* const rppForMe =
                                context->getReqdPhysicalProperty();
    const InputPhysicalProperty* const ippForMe =
                                context->getInputPhysicalProperty();

    // allocate a new memory for multiple substitutes
    memory = new(CmpCommon::statementHeap())
                RuleSubstituteMemory(CmpCommon::statementHeap());

    bef->addIndexInfo();


    // Materialize the disjunct array from the logical scan's
    // (i.e. bef) selection predicates:

    // Must always generate disjuncts, even if MDAM will not be
    // considered.  The 'common' disjunct is used to produce the begin
    // and end keys for the single subset case.
    //
    ValueIdSet inSet(bef->selectionPred());
    const ValueIdSet &generatedComputedColPreds = 
      bef->getComputedPredicates();

    inSet += generatedComputedColPreds;
    Disjuncts *disjunctsPtr =  new (CmpCommon::statementHeap())
      MaterialDisjuncts(inSet);
    Disjuncts *disjunctsPtr0 = disjunctsPtr;
    CMPASSERT(disjunctsPtr);
    CMPASSERT(disjunctsPtr0);
    // -----------------------------------------------------------------
    // Now create all the FileScans for this scan:
    // -----------------------------------------------------------------


    CollIndex numIndexOnlyIndexes = bef->getIndexOnlyIndexes().entries();

    LIST(IndexProperty *) viableIndexes(CmpCommon::statementHeap());
    LIST(OrderComparison) comparisonSet(CmpCommon::statementHeap());


    OrderComparison oc =
	  bef->forceInverseOrder() ? INVERSE_ORDER : SAME_ORDER;
    CollIndex entry =0;
    NABoolean isStream = before->getGroupAttr()->isStream();

    const IndexDesc* fastDeleteIndexDesc = 
        CURRSTMT_OPTDEFAULTS->getRequiredScanDescForFastDelete();

    //No predicates and no requirements just select the smallest index
    if(	CURRSTMT_OPTDEFAULTS->indexEliminationLevel() != OptDefaults::MINIMUM AND
	resultNeedsToBeOrdered == FALSE AND
	(ippForMe == NULL OR ippForMe->getAssumeSortedForCosting()) AND
	rppForMe->getDp2SortOrderPartReq() == NULL AND
	NOT  isStream AND
	bef->selectionPred().isEmpty() AND
        fastDeleteIndexDesc == NULL
      )
    {
      IndexDesc * smallestIndex;
      if(bef->getIndexOnlyIndexes().entries() >1)
	smallestIndex = findSmallestIndex(
					    bef->deriveIndexOnlyIndexDesc());
      else
	smallestIndex = bef->deriveIndexOnlyIndexDesc()[0];

      createAndInsertScan(smallestIndex,bef,memory,disjunctsPtr,generatedComputedColPreds,oc,MDAM_OFF,isHbase);

    }
    else
    {
      for (CollIndex i = 0; i < numIndexOnlyIndexes; i++)
      {
	NABoolean indexQualifies = TRUE;

        // hbase does not support inverse order scan
	oc =
	  (!isHbase && bef->forceInverseOrder()) ? INVERSE_ORDER : SAME_ORDER;

	IndexProperty *ixProp = bef->getIndexOnlyIndexes()[i];
	IndexDesc * idesc = ixProp->getIndexDesc();

        if ( fastDeleteIndexDesc && fastDeleteIndexDesc != idesc )
           continue;

	if (idesc->isClusteringIndex())
	  {
	    const PartitioningFunction* physicalPartFunc =
	      idesc->getPartitioningFunction();
	    
	    // if an explicit partition range has been specified 
	    // as part of the table name to restrict the scan on user
	    // specified partitions, then disable index scan.
	    // This is needed since the index and base table partitions may
	    // not be distributed the same way and an index only scan may not
	    // restrict the partitions as intended by user.
	    // Return the clustering index.
	    if (physicalPartFunc && 
		physicalPartFunc->partitionRangeRestricted())
	      {
		viableIndexes.clear();
		comparisonSet.clear();
		viableIndexes.insert(ixProp);
		comparisonSet.insert(oc);
		
		break;
	      }
	  }

	// if an ordering is required, the index must supply the required
	// order or arrangement
	if (resultNeedsToBeOrdered)
	{
	  ValueIdList sortKey = idesc->getOrderOfKeyValues();

	  // Make sure it satisfies the required order and also
	  // determine the scan direction. If computed column predicates
          // select only one salt or division value, for example,
          // then make use of this to satisfy order requirements.
          if ((rppForMe->getSortKey() != NULL) AND
              ((oc = sortKey.satisfiesReqdOrder(
                       *rppForMe->getSortKey(),
                       before->getGroupAttr(),
                       &bef->getComputedPredicates())) == DIFFERENT_ORDER))
            indexQualifies = FALSE;

          // hbase does not support inverse order scan
          if ( oc == INVERSE_ORDER && isHbase )
            indexQualifies = FALSE;
	  
	  // make sure it satisfies the required arrangement
          if (indexQualifies AND
              (rppForMe->getArrangedCols() != NULL) AND
              NOT sortKey.satisfiesReqdArrangement(
                    *rppForMe->getArrangedCols(),
                    before->getGroupAttr(),
                    &bef->getComputedPredicates()))
            indexQualifies = FALSE;
	}

	// If there is a DP2 sort order partitioning requirement, make
	// sure the physical partitioning function of the index matches it.
	if (indexQualifies AND
	    (rppForMe->getDp2SortOrderPartReq() != NULL))
	{
	  const PartitioningFunction* physicalPartFunc =
          idesc->getPartitioningFunction();
	  if (physicalPartFunc == NULL)
	  {
	    physicalPartFunc = new(CmpCommon::statementHeap())
            SinglePartitionPartitioningFunction();
	  }

	  if (NOT rppForMe->getDp2SortOrderPartReq()->partReqAndFuncCompatible(
	       physicalPartFunc))
	    indexQualifies = FALSE;

	}

        // Skip NJ with Hive table as the inner for now
        //
        //if (isHiveTable AND (ippForMe != NULL))
        //  indexQualifies = FALSE;

	// If there are input physical properties, the index must be
	// able to use the outer table ordering.
	if (indexQualifies AND (ippForMe != NULL)
	    AND (!(ippForMe->getAssumeSortedForCosting())))
	{
	  CMPASSERT((ippForMe->getNjOuterOrder() != NULL) AND
                  NOT ippForMe->getNjOuterOrder()->isEmpty());
	  CMPASSERT(ippForMe->getNjOuterOrderPartFunc() != NULL);

	  const PartitioningFunction* physicalPartFunc =
	    idesc->getPartitioningFunction();
	  if (physicalPartFunc == NULL)
	  {
	    physicalPartFunc = new(CmpCommon::statementHeap())
	      SinglePartitionPartitioningFunction();
	  }

	  // If the outer order is a DP2 sort order, then the
	  // njDp2OuterOrderPartFunc and the index partitioning
	  // function must match exactly or the outer order cannot be used.
	  const PartitioningFunction* njDp2OuterOrderPartFunc =
	    ippForMe->getNjDp2OuterOrderPartFunc();
	  if ((njDp2OuterOrderPartFunc != NULL) AND
	     (njDp2OuterOrderPartFunc->
	        comparePartFuncToFunc(*physicalPartFunc) != SAME))
	    indexQualifies = FALSE;

	  // To be able to use an outer table ordering, the outer
	  // table partitioning function must be a replicateNoBroadcast
	  // partitioning function or must be a grouping of the
	  // index partitioning function.
	  const PartitioningFunction* njOuterOrderPartFunc =
	    ippForMe->getNjOuterOrderPartFunc();
	  if (indexQualifies AND
	     NOT njOuterOrderPartFunc->
                  isAReplicateNoBroadcastPartitioningFunction() AND
	      NOT njOuterOrderPartFunc->isAGroupingOf(*physicalPartFunc) )
	    indexQualifies = FALSE;

	  if (indexQualifies)
	  {
	    // Don't create a plan with this index unless it can use
	    // the outer table order for reducing it's I/O cost. In
	    // other words, the outer table order - i.e. the probes
	    // order - must be at least partially in the same order
	    // as the index sort key columns that are covered by
	    // equijoin predicates.

	    // Determine which columns of the index sort key are
	    // equijoin columns, up to the first column not covered
	    // by a constant or equijoin column.
	    ValueIdList sortKey = idesc->getOrderOfKeyValues();
	    ValueIdList uncoveredCols;
	    ValueIdList equiJoinCols =
	      sortKey.findNJEquiJoinCols(
              ippForMe->getNjOuterCharOutputs(),
              before->getGroupAttr()->getCharacteristicInputs(),
              uncoveredCols);

	    if (equiJoinCols.isEmpty())
	    {
	      // Can't use outer table order for this index if it doesn't
	      // have any leading equijoin columns.
	      indexQualifies = FALSE;
	    }
	    else
	    {
	      // Determine if the leading equijoin column and the leading
	      // column of the outer order are the same.
	      ValueIdList njOuterOrder = *(ippForMe->getNjOuterOrder());

	      // Remove any inverse node on the leading equijoin column
	      // and remember if there was one.
	      ValueId equiJoinCol = equiJoinCols[0];
	      ValueId noInverseEquiJoinCol =
              equiJoinCol.getItemExpr()->removeInverseOrder()->getValueId();
	      NABoolean equiJoinColIsDesc = FALSE;
	      if (noInverseEquiJoinCol != equiJoinCol)
		equiJoinColIsDesc = TRUE;

	      // Remove any inverse node on the leading outer order column
	      // and remember if there was one.
	      ValueId outerOrderCol = njOuterOrder[0];
	      ValueId noInverseOuterOrderCol =
		outerOrderCol.getItemExpr()->removeInverseOrder()->getValueId();
	      NABoolean outerOrderColIsDesc = FALSE;
	      if (noInverseOuterOrderCol != outerOrderCol)
		outerOrderColIsDesc = TRUE;

	      // Leading equijoin column of the index sort key and the
	      // leading column of the outer table sort key must be
	      // the same. If one is DESC, they must both be DESC.
	      if ((noInverseEquiJoinCol != noInverseOuterOrderCol) OR
	         (equiJoinColIsDesc != outerOrderColIsDesc))
		indexQualifies = FALSE;

	    } // end if index key has leading equijoin cols
	  } // end if index still qualifies
	} // end if ipp exist and index still qualifies


	// QSTUFF
	if (indexQualifies && before->getGroupAttr()->isStream())
	{
	  if (NOT idesc->isClusteringIndex())
          {
	    // we only test whether the condition holds for a secondary
	    // index, the cluster index, i.e. the base table will produce
	    // all output values
	    ValueIdSet outputs =
              (before->getGroupAttr()->isEmbeddedUpdate() ?
               before->getGroupAttr()->getGenericUpdateRootOutputs() :
               before->getGroupAttr()->getCharacteristicOutputs());

            ValueIdList vegcolumns;

            ((Scan *)before)->getTableDesc()->
              getEquivVEGCols(idesc->getIndexColumns(),vegcolumns);

            ValueIdSet columns(vegcolumns);
            outputs.removeCoveredExprs(columns);


            if (NOT (outputs.isEmpty() ||
              before->getGroupAttr()->isEmbeddedDelete()))
              {
                *CmpCommon::diags() << DgSqlCode(4207)
                  << DgTableName(idesc->getNAFileSet()->getExtFileSetName());

                indexQualifies = FALSE;
              }
          }

        if (indexQualifies &&
            resultNeedsToBeOrdered &&
            idesc->isPartitioned())
          {
            // 10-010109-0583 "select ... from stream ...order by" causes hang
            // if stream is partn'd."

            //The following check is made to avoid insertion of
            //duplicate combination of error number 4212 and its
            //associated extFileSetName.
            if(!((*CmpCommon::diags()).containsForFile(4212,
			((idesc->getNAFileSet()->getExtFileSetName()).data()))))
               *CmpCommon::diags() << DgSqlCode(4212)
                      << DgTableName(idesc->getNAFileSet()->getExtFileSetName());

            indexQualifies = FALSE;
          }

         //Do not consider plans which have stream access on partitioned
         //access paths when the flag ATTEMPT_ASYNCHRONOUS_ACCESS is
         //OFF. This is because, the split top operator is not used.
         //Instead, a partition access operator is used, but the PA handles
         //multiple partitions by looking for the end-of-data from the current
         //partition, and then sending the request to the next partition.
         //For streams, there is no end-of-data.

        if(indexQualifies &&
	    isStream &&
	     idesc->isPartitioned() &&
              CmpCommon::getDefault(ATTEMPT_ASYNCHRONOUS_ACCESS) == DF_OFF)
	  {
	   if(!((*CmpCommon::diags()).containsForFile(4320,
               ((idesc->getNAFileSet()->getExtFileSetName()).data()))))
               *CmpCommon::diags() << DgSqlCode(4320)
               << DgTableName(idesc->getNAFileSet()->getExtFileSetName());

             indexQualifies = FALSE;
         }
     }
      // QSTUFF

	if (indexQualifies)
	{

	  viableIndexes.insert(ixProp);
	  comparisonSet.insert(oc);


	}  // if index qualifies
      } // for every index
    }
  if(viableIndexes.entries() ==1)
  {
    ValueIdSet keyColSet(viableIndexes[0]->getIndexDesc()->getIndexKey());
    usePartofSelectionPredicatesFromTheItemExpressionTree(inSet,keyColSet.convertToBaseIds());
    disjunctsPtr0 =  new (CmpCommon::statementHeap())
      MaterialDisjuncts(inSet);
    CMPASSERT(disjunctsPtr0);

    createAndInsertScan(viableIndexes[0]->getIndexDesc(),bef,memory,disjunctsPtr0,generatedComputedColPreds,
		        comparisonSet[0],viableIndexes[0]->getMdamFlag(),isHbase);
  }

  else if(	CURRSTMT_OPTDEFAULTS->indexEliminationLevel() != OptDefaults::MINIMUM AND
	NOT isStream AND
	bef->selectionPred().isEmpty() AND viableIndexes.entries() >1)
    {
      //No predicates so select the smallest order/part satisfying index. All the
      //index that are in the viable index set satisfy requirements naturally.
      IndexDesc * smallestIndex = findSmallestIndex(viableIndexes,entry);
      createAndInsertScan(smallestIndex,bef,memory,disjunctsPtr,generatedComputedColPreds,
                          comparisonSet[entry],MDAM_OFF, isHbase);
      //if it is under a nested join then select the index that most number of
      //partitions.
      if((ippForMe!=NULL) AND (!(ippForMe->getAssumeSortedForCosting())))
      {
	IndexDesc * partitionedIndex =
	      findMostPartitionedIndex(viableIndexes,entry);
	if(partitionedIndex != smallestIndex)
	{
	  createAndInsertScan(partitionedIndex,bef,memory,disjunctsPtr,
			      generatedComputedColPreds,comparisonSet[entry],MDAM_OFF, isHbase);
	}
      }
    }
    else //there are predicates
    {

      //eliminate indexes with bad key access and bad index joins
      CollIndex numIndex = viableIndexes.entries();
      if(CURRSTMT_OPTDEFAULTS->indexEliminationLevel() == OptDefaults::MAXIMUM AND
	numIndex >1 AND NOT isStream)
      {
	  removeLowSelectivityIndexJoins(viableIndexes,comparisonSet,context);
	if(viableIndexes.entries() >1)
	  removeBadKeyIndexes(viableIndexes,comparisonSet);
      }

      numIndex = viableIndexes.entries();
      for(CollIndex j=0;j<numIndex;j++)
      {
	createAndInsertScan(viableIndexes[j]->getIndexDesc(),bef,memory,disjunctsPtr0,
		            generatedComputedColPreds,comparisonSet[j],viableIndexes[j]->getMdamFlag(), isHbase);
      }

    }
  }


  // ---------------------------------------------------------------------
  // handle case of multiple substitutes
  // ---------------------------------------------------------------------
  if (memory)
  {
    result = memory->getNextSubstitute();

    if (result == NULL)
    {
      // returned all the substitutes
      // now delete the substitute memory, so we won't be called again
      delete memory;
      memory = NULL;
    }

    // return the next retrieved substitute
    return result;
  }
  else
    return NULL; // rule didn't fire
}

RelExpr * FileScanRule::nextSubstitute(RelExpr * before,
                                      Context * context,
                                      RuleSubstituteMemory *& memory)
{
   return generateScanSubstitutes(before, context, memory, FALSE);
}


// -----------------------------------------------------------------------
// methods for class HbaseScanRule
// -----------------------------------------------------------------------

HbaseScanRule::~HbaseScanRule() {} 

NABoolean HbaseScanRule::topMatch(RelExpr * relExpr, Context *context)
{
  if (NOT Rule::topMatch(relExpr,context))
    return FALSE;

  Scan * scan= (Scan *) relExpr;
  if (scan->getTableDesc()->getNATable()->isHbaseTable() == FALSE)
    return FALSE;

  const ReqdPhysicalProperty* rppForMe = context->getReqdPhysicalProperty();

  // Hbase table scan executes in master or ESP
  if (rppForMe->executeInDP2())
      return FALSE;

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  if (relExpr->rppRequiresEnforcer(context->getReqdPhysicalProperty()))
    return FALSE;


  return TRUE;
}

RelExpr * HbaseScanRule::nextSubstitute(RelExpr * before,
                                        Context * context,
                                        RuleSubstituteMemory *& memory)
{
   return generateScanSubstitutes(before, context, memory, TRUE);
}


// -----------------------------------------------------------------------
// methods for class UnionRule
// -----------------------------------------------------------------------

UnionRule::~UnionRule() {} 

NABoolean UnionRule::topMatch(RelExpr * relExpr, Context * context)
{

  if (NOT Rule::topMatch(relExpr,context))
    return FALSE;

  if (((Union *)relExpr)->getSerialUnion() &&
      context->getReqdPhysicalProperty()->getPlanExecutionLocation() != EXECUTE_IN_MASTER)
  {
    return FALSE;
  }

  // QSTUFF
  CMPASSERT(NOT(relExpr->getGroupAttr()->isEmbeddedUpdateOrDelete()));
  // QSTUFF

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  if (relExpr->rppRequiresEnforcer(context->getReqdPhysicalProperty()))
    return FALSE;

  // -----------------------------------------------------------------
  // Check whether the union can potentially satisfy the required
  // physical properties.
  // -----------------------------------------------------------------
  if (NOT ((Union *)relExpr)->rppAreCompatibleWithOperator
                     (context->getReqdPhysicalProperty()))
    return FALSE;

  return TRUE;

} // UnionRule::topMatch

RelExpr * UnionRule::nextSubstitute(RelExpr * before,
                                    Context *  /* context */,
                                    RuleSubstituteMemory *& /*memory*/)
{
  MergeUnion *result;
  Union *bef = (Union *) before;

  CMPASSERT(bef->getOperatorType() == REL_UNION);

  // a union node must not have any predicates
  CMPASSERT(bef->getSelectionPred().entries() == 0);

  // return a physical merge union node
  result = new(CmpCommon::statementHeap()) MergeUnion(bef->child(0),
                                                      bef->child(1),
                                                      bef->getUnionMap());

  (void) bef->copyTopNode(result, CmpCommon::statementHeap());

  // now set the attributes of the result's top node
  result->setCondExpr(bef->getCondExpr());
  result->setAlternateRightChildOrderExpr(bef->getAlternateRightChildOrderExpr()); //++ MV
  result->setGroupAttr(bef->getGroupAttr());
  result->setUnionFlags(bef->getUnionFlags());
  result->setControlFlags(bef->getControlFlags()); //++ Triggers -
  result->setBlockStmt(before->isinBlockStmt());

  return result;
}

// -----------------------------------------------------------------------
// methods for class SortGroupByRule
// -----------------------------------------------------------------------

SortGroupByRule::~SortGroupByRule() {} 

NABoolean SortGroupByRule::topMatch (RelExpr *relExpr,
                                     Context *context)
{
  // check if this rule has been disabled via RuleGuidanceCQD
  // the CQD is COMP_INT_77 and it represents a bitmap
  // below we check if the bit # 7 is ON
  if(CURRSTMT_OPTDEFAULTS->isRuleDisabled(7))
    return FALSE;

  if (NOT Rule::topMatch(relExpr,context))
    return FALSE;

  if (relExpr->getOperatorType() == REL_SHORTCUT_GROUPBY)
    return FALSE;

  CMPASSERT(relExpr->getOperatorType() == REL_GROUPBY);

  // QSTUFF
  CMPASSERT(NOT(relExpr->getGroupAttr()->isStream() OR
                relExpr->getGroupAttr()->isEmbeddedUpdateOrDelete()));
  // QSTUFF

  GroupByAgg *grbyagg = (GroupByAgg *) relExpr;

  // Don't apply this rule for aggregate queries with no grouping
  // columns - fire the Aggregate Rule instead.
  if (grbyagg->groupExpr().isEmpty())
    return FALSE;

  // must use sortGroupBy for rollup aggregates
  if (grbyagg->isRollup())
    return TRUE;

  // Settings to limit Sort Group By application
  Lng32 sortGbySetting = CURRSTMT_OPTDEFAULTS->robustSortGroupBy();
  if (context->getReqdPhysicalProperty()->getMustMatch() == NULL &&
      sortGbySetting > 0)
  {
    if (grbyagg->isAPartialGroupByRoot())
    {
      // disallow sortGroupBy from partialGrpByRoot if no order requirement
      if (sortGbySetting >= 1 && !context->requiresOrder())
        return FALSE;

      // disallow sortGroupBy from partialGrpByRoot if requested
    if (sortGbySetting >= 2)
        return FALSE;
    }

    // disallow sortGroupBy in ESP if requested
    if (!context->getReqdPhysicalProperty()->executeInDP2() &&
        sortGbySetting >= 3)
        return FALSE;
  }

  // Do not apply this rule if the Group By elimination rule
  // can be applied.
  if ( grbyagg->isNotAPartialGroupBy() &&
       grbyagg->child(0).getGroupAttr()->isUnique(grbyagg->groupExpr()) )
    return FALSE;

  // can't use this algorithm if there are distinct aggregates
  const ValueIdSet &aggrs = grbyagg->aggregateExpr();
  for (ValueId x = aggrs.init(); aggrs.next(x); aggrs.advance(x))
    {
      Aggregate *agg = (Aggregate *) x.getItemExpr();

      CMPASSERT(x.getItemExpr()->isAnAggregate());

      if (agg->isDistinct())
        {
          ValueIdSet uniqueSet = grbyagg->groupExpr();
          uniqueSet += agg->getDistinctValueId();
          if (NOT grbyagg->child(0).getGroupAttr()->isUnique(uniqueSet))
            return FALSE;
        }
    }

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  if (relExpr->rppRequiresEnforcer(context->getReqdPhysicalProperty()))
    return FALSE;

  // Test the eligibility of creating a plan using the given
  // partitioning and location requirements.
  if (NOT grbyagg->rppAreCompatibleWithOperator
                     (context->getReqdPhysicalProperty()))
    return FALSE;

  // if at most 1 row is being returned, then the rule matches
  if (relExpr->getGroupAttr()->getMaxNumOfRows() <= 1)
    return TRUE;


  return TRUE;

} // SortGroupByRule::topMatch()

RelExpr * SortGroupByRule::nextSubstitute(RelExpr * before,
                                          Context * /*context*/,
                                          RuleSubstituteMemory *& /*memory*/)
{
  SortGroupBy *result;
  GroupByAgg *bef = (GroupByAgg *) before;

  // create a sort groupby node
  result = new(CmpCommon::statementHeap()) SortGroupBy(bef->child(0));

  // now set the group attributes of the result's top node
  result->setGroupAttr(bef->getGroupAttr());

  (void) bef->copyTopNode(result, CmpCommon::statementHeap());

  // the required order is determined by SortGroupBy::createContextForAChild()

  return result;
} // SortGroupByRule::nextSubstitute()

// -----------------------------------------------------------------------
// methods for class AggregateRule
// -----------------------------------------------------------------------
AggregateRule::~AggregateRule() {} 

NABoolean AggregateRule::topMatch (RelExpr *relExpr,
                                   Context *context)
{
  if (NOT Rule::topMatch(relExpr,context))
    return FALSE;

  if (relExpr->getOperatorType() == REL_SHORTCUT_GROUPBY)
    return FALSE;

  CMPASSERT(relExpr->getOperatorType() == REL_GROUPBY);

  // QSTUFF
  CMPASSERT(NOT(relExpr->getGroupAttr()->isStream() OR
                relExpr->getGroupAttr()->isEmbeddedUpdateOrDelete()));
  // QSTUFF


  GroupByAgg *grbyagg = (GroupByAgg *) relExpr;

  // Don't apply this rule if there are grouping columns -
  // apply the SortGroupByRule instead.
  if (NOT grbyagg->groupExpr().isEmpty())
    return FALSE;

  // can't use this algorithm if there are distinct aggregates
  // that aren't fake distincts
  const ValueIdSet &aggrs = grbyagg->aggregateExpr();
  for (ValueId x = aggrs.init(); aggrs.next(x); aggrs.advance(x))
    {
      Aggregate *agg = (Aggregate *) x.getItemExpr();

      CMPASSERT(x.getItemExpr()->isAnAggregate());

      if (agg->isDistinct())
        {
          if (NOT grbyagg->child(0).getGroupAttr()->
                             isUnique(agg->getDistinctValueId()))
            return FALSE;
        }
    }

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  if (relExpr->rppRequiresEnforcer(context->getReqdPhysicalProperty()))
    return FALSE;

  // Test the eligibility of creating a plan using the given
  // partitioning and location requirements.
  if (NOT grbyagg->rppAreCompatibleWithOperator(
                     context->getReqdPhysicalProperty()))
    return FALSE;

  return TRUE;

} // AggregateRule::topMatch()

// -----------------------------------------------------------------------
// methods for class PhysShortCutGroupByRule
// -----------------------------------------------------------------------

PhysShortCutGroupByRule::~PhysShortCutGroupByRule() {} 

NABoolean PhysShortCutGroupByRule::topMatch (RelExpr *relExpr,
                                             Context *context)
{
  if (NOT Rule::topMatch(relExpr,context))
    return FALSE;

  // make sure we are looking at a short cut group by
  CMPASSERT(relExpr->getOperatorType() == REL_SHORTCUT_GROUPBY);

  // Cast the relexpr pointer to groupbyagg type
  GroupByAgg *grbyagg = (GroupByAgg *) relExpr;

  // The ShortCutGroupBy transformation rule should have already
  // verified that this group by has no group by clause.
  CMPASSERT (grbyagg->groupExpr().isEmpty());

  // The ShortCutGroupBy transformation rule already verified that
  // this group by has an aggregate. Determine what type of aggregate.
  const ValueIdSet &aggrs = grbyagg->aggregateExpr();
  CMPASSERT (NOT aggrs.isEmpty());
  ValueId aggr_valueid = aggrs.init();
  aggrs.next(aggr_valueid);
  ItemExpr *item_expr = aggr_valueid.getItemExpr();
  OperatorTypeEnum aggrType = item_expr->getOperatorType();

  if(aggrType==ITM_ANY_TRUE)
  {
    // AnyTrueGroupByAgg can only be executed in ESP.
    if (context->getReqdPhysicalProperty()->executeInDP2())
      return FALSE;
  }

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  if (relExpr->rppRequiresEnforcer(context->getReqdPhysicalProperty()))
    return FALSE;

  // Test the eligibility of creating a plan using the given
  // partitioning and location requirements.
  if (NOT grbyagg->rppAreCompatibleWithOperator
                     (context->getReqdPhysicalProperty()))
    return FALSE;

  return TRUE;

} // PhysShortCutGroupByRule::topMatch()

RelExpr * PhysShortCutGroupByRule::nextSubstitute(RelExpr * before,
                                     Context * /*context*/,
                                     RuleSubstituteMemory *& /*memory*/)
{
  PhysShortCutGroupBy *result;
  ShortCutGroupBy *bef = (ShortCutGroupBy *) before;

  // create a physical shortcut groupby node
  result = new(CmpCommon::statementHeap()) PhysShortCutGroupBy(bef->child(0));

  // now set the group attributes of the result's top node
  result->setGroupAttr(bef->getGroupAttr());

  // Copy over the shortcut groupby private fields, then
  // call the groupbyagg copytopnode to copy the groupby private fields,
  // then call the relexpr copytopnode to copy over all common fields.
  (void) bef->copyTopNode(result, CmpCommon::statementHeap());

  return result;

} // PhysShortCutGroupByRule::nextSubstitute()


// -----------------------------------------------------------------------
// methods for class HashGroupByRule
// -----------------------------------------------------------------------

HashGroupByRule::~HashGroupByRule() {} 

NABoolean HashGroupByRule::topMatch (RelExpr *relExpr,
                                     Context *context)
{
  // check if this rule has been disabled via RuleGuidanceCQD
  // the CQD is COMP_INT_77 and it represents a bitmap
  // below we check if the bit # 6 is ON
  if(CURRSTMT_OPTDEFAULTS->isRuleDisabled(6))
    return FALSE;

  if (NOT Rule::topMatch(relExpr,context))
    return FALSE;

  if (relExpr->getOperatorType() == REL_SHORTCUT_GROUPBY)
    return FALSE;

  CMPASSERT(relExpr->getOperatorType() == REL_GROUPBY);

  GroupByAgg *grbyagg = (GroupByAgg *) relExpr;

  // QSTUFF
  CMPASSERT(NOT(relExpr->getGroupAttr()->isStream() OR
                relExpr->getGroupAttr()->isEmbeddedUpdateOrDelete()));
  // QSTUFF


  // don't apply this rule for aggregate queries
  // a sort group by will do a better job
  if (grbyagg->groupExpr().isEmpty())
    return FALSE;

  // Do not apply this rule if the Group By elimination rule
  // can be applied.
  if ( grbyagg->isNotAPartialGroupBy() &&
       grbyagg->child(0).getGroupAttr()->isUnique(grbyagg->groupExpr()) )
    return FALSE;

  // can't use this algorithm if there are distinct aggregates
  const ValueIdSet &aggrs = grbyagg->aggregateExpr();

  for (ValueId x = aggrs.init(); aggrs.next(x); aggrs.advance(x))
    {
      Aggregate *agg = (Aggregate *) x.getItemExpr();

      CMPASSERT(x.getItemExpr()->isAnAggregate());

      //if it is pivot_group(), currently, hash groupby is not supported
      if (agg->getOperatorType() == ITM_PIVOT_GROUP)
        return FALSE;

      if (agg->isDistinct())
        {
          ValueIdSet uniqueSet = grbyagg->groupExpr();
         uniqueSet += agg->getDistinctValueId();
          if (NOT grbyagg->child(0).getGroupAttr()->isUnique(uniqueSet))
            return FALSE;
        }
    }

  // a hash groupby doesn't produce any useful ordering
  if (context->requiresOrder())
    return FALSE;

  // a hash group cannot be pushed completely to DP2, since overflow
  // cannot be handled in DP2.
  if (grbyagg->isNotAPartialGroupBy() AND
      context->getReqdPhysicalProperty()->executeInDP2())
    return FALSE;

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  if (relExpr->rppRequiresEnforcer(context->getReqdPhysicalProperty()))
    return FALSE;

  // Test the eligibility of creating a plan using the given
  // partitioning and location requirements.
  if (NOT grbyagg->rppAreCompatibleWithOperator(
                     context->getReqdPhysicalProperty()))
    return FALSE;

  // groupby rollup is evaluated using SortGroupBy
  if (grbyagg->isRollup())
    return FALSE;

  return TRUE;

} // HashGroupByRule::topMatch()

RelExpr * HashGroupByRule::nextSubstitute(RelExpr * before,
                                          Context * context,
                                          RuleSubstituteMemory *& memory)
{
  HashGroupBy *result;
  GroupByAgg *bef = (GroupByAgg *) before;

  // return a physical node
  result = (HashGroupBy *) Rule::nextSubstitute(before,context,memory);

  (void) bef->copyTopNode(result, CmpCommon::statementHeap());

  return result;
} // HashGroupByRule::nextSubstitute()

// -- MVs
// This method checks is the data to be inserted is ordered according to the
// table's clustering index, so that VSBB insert can be used.
// The code was mostly adapted from CostMethodDP2Insert::computeOperatorCost()
NABoolean Insert::isDataSorted(Insert *bef, const Context *context)
{
  NABoolean probesForceSynchronousAccess; // an unused parameter for ordersMatch().

  const IndexDesc* indexDesc = bef->getTableDesc()->getClusteringIndex();
  ValueIdList targetSortKey = indexDesc->getOrderOfKeyValues();

  // If a target key column is covered by a constant on the source side,
  // then we need to remove that column from the target sort key
  removeConstantsFromTargetSortKey(&targetSortKey,
	                           &(bef->updateToSelectMap()));

  ValueIdSet sourceCharInputs =
    bef->getGroupAttr()->getCharacteristicInputs();
  ValueIdSet targetCharInputs;
  // The char inputs are still in terms of the source. Map them to the target.
  // Note: The source char outputs in the ipp have already been mapped to
  // the target.
  bef->updateToSelectMap().rewriteValueIdSetUp(targetCharInputs,
					       sourceCharInputs);

  return ordersMatch(context->getInputPhysicalProperty(),
		     indexDesc,
		     &targetSortKey,
		     targetCharInputs,
		     FALSE,
		     probesForceSynchronousAccess,
		     TRUE);
}

// -----------------------------------------------------------------------
// methods for class HbaseDeleteRule
// -----------------------------------------------------------------------

HbaseDeleteRule::~HbaseDeleteRule() {} 

NABoolean HbaseDeleteRule::topMatch(RelExpr * relExpr, Context *context)
{
  //  if (NOT ((GenericUpdate *)relExpr)->selectionPred().isEmpty())
  //    return FALSE;

  if (NOT Rule::topMatch(relExpr,context))
    return FALSE;

  Delete * del = (Delete *) relExpr;
  if (del->getTableDesc()->getNATable()->isHbaseTable() == FALSE)
    return FALSE;

  if (del->getTableDesc()->getNATable()->hasLobColumn())
    return FALSE;
  
  // HbaseDelete can only execute above DP2
  if (context->getReqdPhysicalProperty()->executeInDP2())
    return FALSE;

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  //  if (relExpr->rppRequiresEnforcer(context->getReqdPhysicalProperty()))
  // return FALSE;

  CostScalar numberOfRowsToBeDeleted =
    context->getInputLogProp()->getResultCardinality();

  if ((numberOfRowsToBeDeleted > 1) &&
      (NOT del->noCheck()) &&
      (CmpCommon::getDefault(HBASE_SQL_IUD_SEMANTICS) == DF_ON) &&
      (CmpCommon::getDefault(HBASE_UPDEL_CURSOR_OPT) == DF_ON) &&
      ((ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_74) == 0))
    return FALSE;

  return TRUE;

}

RelExpr * HbaseDeleteRule::nextSubstitute(RelExpr * before,
                                        // QSTUFF
                                        Context * context,
                                        // QSTUFF
                                        RuleSubstituteMemory *& /*memory*/)
{
  HbaseDelete * result;
  Delete * bef  = (Delete *) before;
  Scan* scan = (Scan*) before->child(0).getPtr();

  // This transformation can be performed only if the table scanned
  // is the same as that being updated.
  if (scan->getTableDesc()->getNATable() != bef->getTableDesc()->getNATable())
    return NULL;

  // Build the new HbaseDelete node
  result = new(CmpCommon::statementHeap())
    HbaseDelete(bef->getTableName(), bef->getTableDesc());

  copyCommonGenericUpdateFields(result, bef);
  copyCommonDeleteFields(result, bef);

  /////////////////////////////////////////////////////////////////////////
  // Setup the hbase search key. The logic is modeled based on 
  // SimpleFileScanOptimizer::constructSearchKey()
  /////////////////////////////////////////////////////////////////////////
  ValueIdSet exePreds(scan->getSelectionPred());

  // only get the first member from the index set
  if ( scan->deriveIndexOnlyIndexDesc().entries() == 0 )
    return NULL;

  IndexDesc* idesc = (scan->deriveIndexOnlyIndexDesc())[0];

  ValueIdSet nonKeyColumnSet;
  idesc->getNonKeyColumnSet(nonKeyColumnSet);

  SearchKey * skey = new(CmpCommon::statementHeap())
    SearchKey(idesc->getIndexKey(),
              idesc->getOrderOfKeyValues(),
              bef->getGroupAttr()->getCharacteristicInputs(),
              TRUE, // forward scan
              exePreds,
              nonKeyColumnSet,
              idesc,
              bef->getTableDesc()->getClusteringIndex());
  result->setSearchKey(skey);
  result->executorPred() = exePreds;
  result->beginKeyPred().clear();

  if ((! skey->isUnique()) &&
      (skey->areAllChosenPredsEqualPreds()) &&
      (NOT bef->noCheck()) &&
      (CmpCommon::getDefault(HBASE_SQL_IUD_SEMANTICS) == DF_ON) &&
      (CmpCommon::getDefault(HBASE_UPDEL_CURSOR_OPT) == DF_ON) &&
      ((ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_74) == 0))
    result = NULL;

  return result;
}

// -----------------------------------------------------------------------
// methods for class HbaseDeleteCursorRule
// -----------------------------------------------------------------------

HbaseDeleteCursorRule::~HbaseDeleteCursorRule() {} 

NABoolean HbaseDeleteCursorRule::topMatch(RelExpr * relExpr, Context *context)
{
  //  if (NOT ((GenericUpdate *)relExpr)->selectionPred().isEmpty())
  //    return FALSE;

  if (NOT Rule::topMatch(relExpr,context))
    return FALSE;

  Delete * del = (Delete *) relExpr;
  if (del->getTableDesc()->getNATable()->isHbaseTable() == FALSE)
    return FALSE;
  
  // HbaseDelete can only execute above DP2
  if (context->getReqdPhysicalProperty()->executeInDP2())
    return FALSE;

  CMPASSERT(relExpr->getOperatorType() == REL_LEAF_DELETE);

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  //  if (relExpr->rppRequiresEnforcer(context->getReqdPhysicalProperty()))
  // return FALSE;

  return TRUE;

}

RelExpr * HbaseDeleteCursorRule::nextSubstitute(RelExpr * before,
                                        // QSTUFF
                                        Context * context,
                                        // QSTUFF
                                        RuleSubstituteMemory *& /*memory*/)
{
  HbaseDelete * result;
  Delete * bef  = (Delete *) before;

  // Build the new HbaseDelete node
  result = new(CmpCommon::statementHeap())
    HbaseDelete(bef->getTableName(), bef->getTableDesc());
  result->cursorHbaseOper() = TRUE;

  copyCommonGenericUpdateFields(result, bef);
  copyCommonDeleteFields(result, bef);

  /////////////////////////////////////////////////////////////////////////
  // Setup the hbase search key. The logic is modeled based on 
  // SimpleFileScanOptimizer::constructSearchKey()
  /////////////////////////////////////////////////////////////////////////
  ValueIdSet exePreds(bef->getSelectionPred());
  const IndexDesc* idesc =  bef->getTableDesc()->getClusteringIndex();

  ValueIdSet nonKeyColumnSet;
  idesc->getNonKeyColumnSet(nonKeyColumnSet);

  ValueIdSet beginKeyPreds(bef->getBeginKeyPred());
  exePreds += beginKeyPreds;

  SearchKey * skey = new(CmpCommon::statementHeap())
    SearchKey(idesc->getIndexKey(),
              idesc->getOrderOfKeyValues(),
              bef->getGroupAttr()->getCharacteristicInputs(),
              TRUE, // forward scan
              exePreds,
              nonKeyColumnSet,
              idesc);
  result->setSearchKey(skey);
  result->executorPred() = exePreds;
  result->beginKeyPred().clear();

  return result;
}

// -----------------------------------------------------------------------
// methods for class HbaseUpdateRule
// -----------------------------------------------------------------------

HbaseUpdateRule::~HbaseUpdateRule() {} 

NABoolean HbaseUpdateRule::topMatch(RelExpr * relExpr, Context *context)
{
  //  if (NOT ((GenericUpdate *)relExpr)->selectionPred().isEmpty())
  //    return FALSE;

  if (NOT Rule::topMatch(relExpr,context))
    return FALSE;

  Update * upd = (Update *) relExpr;
  if (upd->getTableDesc()->getNATable()->isHbaseTable() == FALSE)
    return FALSE;
  
  // HbaseUpdate can only execute above DP2
  if (context->getReqdPhysicalProperty()->executeInDP2())
    return FALSE;

  CostScalar numberOfRowsToBeUpdated =
    context->getInputLogProp()->getResultCardinality();

  if ((numberOfRowsToBeUpdated > 1) &&
      (NOT upd->isMerge()) &&
      (CmpCommon::getDefault(HBASE_SQL_IUD_SEMANTICS) == DF_ON) &&
      (CmpCommon::getDefault(HBASE_UPDEL_CURSOR_OPT) == DF_ON))
    return FALSE;

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  //  if (relExpr->rppRequiresEnforcer(context->getReqdPhysicalProperty()))
  // return FALSE;

  return TRUE;

}

RelExpr * HbaseUpdateRule::nextSubstitute(RelExpr * before,
                                        // QSTUFF
                                        Context * context,
                                        // QSTUFF
                                        RuleSubstituteMemory *& /*memory*/)
{
  HbaseUpdate * result;
  Update * bef  = (Update *) before;
  Scan* scan = (Scan*) before->child(0).getPtr();

  // This transformation can be performed only if the table scanned
  // is the same as that being updated.
  if (scan->getTableDesc()->getNATable() != bef->getTableDesc()->getNATable())
    return NULL;

  // Build the new HbaseUpdate node
  result = new(CmpCommon::statementHeap())
    HbaseUpdate(bef->getTableName(), bef->getTableDesc());

  copyCommonGenericUpdateFields(result, bef);
  copyCommonUpdateFields(result, bef);

  /////////////////////////////////////////////////////////////////////////
  // Setup the hbase search key. The logic is modeled based on 
  // SimpleFileScanOptimizer::constructSearchKey()
  /////////////////////////////////////////////////////////////////////////
  ValueIdSet exePreds(scan->getSelectionPred());
  // Use indexdesc of base table, een if the optimizer has chosen an
  // index access path for this soon to be gone scan.
  const IndexDesc* idesc = scan->getTableDesc()->getClusteringIndex();
  ValueIdSet clusteringKeyCols(
       idesc->getClusteringKeyCols());
  ValueIdSet generatedComputedColPreds;

  ValueIdSet nonKeyColumnSet;
  idesc->getNonKeyColumnSet(nonKeyColumnSet);

  if (CmpCommon::getDefault(MTD_GENERATE_CC_PREDS) == DF_ON)
    ScanKey::createComputedColumnPredicates(
         exePreds,
         clusteringKeyCols,
         bef->getGroupAttr()->getCharacteristicInputs(),
         generatedComputedColPreds);

  SearchKey * skey = new(CmpCommon::statementHeap())
    SearchKey(idesc->getIndexKey(),
              idesc->getOrderOfKeyValues(),
              bef->getGroupAttr()->getCharacteristicInputs(),
              TRUE, // forward scan
              exePreds,
              nonKeyColumnSet,
              idesc,
              bef->getTableDesc()->getClusteringIndex());
  result->setSearchKey(skey);
  result->executorPred() = exePreds;
  result->beginKeyPred().clear();

  if ((! skey->isUnique()) &&
      (NOT bef->isMerge()) &&
      (skey->areAllChosenPredsEqualPreds()) &&
      (CmpCommon::getDefault(HBASE_SQL_IUD_SEMANTICS) == DF_ON) &&
      (CmpCommon::getDefault(HBASE_UPDEL_CURSOR_OPT) == DF_ON))
    result = NULL;

  return result;
}

// -----------------------------------------------------------------------
// methods for class HbaseUpdateCursorRule
// -----------------------------------------------------------------------

HbaseUpdateCursorRule::~HbaseUpdateCursorRule() {} 

NABoolean HbaseUpdateCursorRule::topMatch(RelExpr * relExpr, Context *context)
{
  //  if (NOT ((GenericUpdate *)relExpr)->selectionPred().isEmpty())
  //    return FALSE;

  if (NOT Rule::topMatch(relExpr,context))
    return FALSE;

  Update * del = (Update *) relExpr;
  if (del->getTableDesc()->getNATable()->isHbaseTable() == FALSE)
    return FALSE;
  
  // HbaseUpdate can only execute above DP2
  if (context->getReqdPhysicalProperty()->executeInDP2())
    return FALSE;

  CMPASSERT(relExpr->getOperatorType() == REL_LEAF_UPDATE);

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  //  if (relExpr->rppRequiresEnforcer(context->getReqdPhysicalProperty()))
  // return FALSE;

  return TRUE;

}

RelExpr * HbaseUpdateCursorRule::nextSubstitute(RelExpr * before,
                                        // QSTUFF
                                        Context * context,
                                        // QSTUFF
                                        RuleSubstituteMemory *& /*memory*/)
{
  HbaseUpdate * result;
  Update * bef  = (Update *) before;

  // Build the new HbaseUpdate node
  result = new(CmpCommon::statementHeap())
    HbaseUpdate(bef->getTableName(), bef->getTableDesc());
  result->cursorHbaseOper() = TRUE;

  copyCommonGenericUpdateFields(result, bef);
  copyCommonUpdateFields(result, bef);

  /////////////////////////////////////////////////////////////////////////
  // Setup the hbase search key. The logic is modeled based on 
  // SimpleFileScanOptimizer::constructSearchKey()
  /////////////////////////////////////////////////////////////////////////
  ValueIdSet exePreds(bef->getSelectionPred());
  const IndexDesc* idesc = bef->getTableDesc()->getClusteringIndex();

  ValueIdSet nonKeyColumnSet;
  idesc->getNonKeyColumnSet(nonKeyColumnSet);

  ValueIdSet beginKeyPreds(bef->getBeginKeyPred());
  exePreds += beginKeyPreds;

  SearchKey * skey = new(CmpCommon::statementHeap())
    SearchKey(idesc->getIndexKey(),
              idesc->getOrderOfKeyValues(),
              bef->getGroupAttr()->getCharacteristicInputs(),
              TRUE, // forward scan
              exePreds,
              nonKeyColumnSet,
              idesc);
  result->setSearchKey(skey);
  result->executorPred() = exePreds;
  result->beginKeyPred().clear();

  return result;
}

// -----------------------------------------------------------------------
// methods for class HiveInsertRule
// -----------------------------------------------------------------------
HiveInsertRule::~HiveInsertRule() {} 

NABoolean HiveInsertRule::topMatch(RelExpr * relExpr, Context *context)
{
  if (NOT Rule::topMatch(relExpr,context))
    return FALSE;

  CMPASSERT(relExpr->getOperatorType() == REL_LEAF_INSERT);
  Insert * ins= (Insert *) relExpr;

  if (ins->getTableDesc()->getNATable()->isHiveTable() == FALSE)
    return FALSE;

  // HiveInsert can only execute above DP2.
  if (context->getReqdPhysicalProperty()->executeInDP2())
    return FALSE;

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  //if (relExpr->rppRequiresEnforcer(context->getReqdPhysicalProperty()))
  //  return FALSE;

  return TRUE;
}

RelExpr * HiveInsertRule::nextSubstitute(RelExpr * before,
                                              Context * context,
                                              RuleSubstituteMemory *& /*mem*/)
{
  CMPASSERT(before->getOperatorType() == REL_LEAF_INSERT);
  CMPASSERT(before->getArity() == 0);

  HiveInsert * result;
  Insert * bef = (Insert *) before;

  // build the key for the insert from the expression for the new record
  SearchKey * skey = NULL;
  const IndexDesc * CIdesc = bef->getTableDesc()->getClusteringIndex();
  const PartitioningFunction* insPartFunc = CIdesc->getPartitioningFunction();

  ValueIdSet externalInputs = before->getGroupAttr()->getCharacteristicInputs();

   NABoolean rangeLoggingRequired =
    bef->getTableDesc()->getNATable()->getMvAttributeBitmap().getAutomaticRangeLoggingRequired();

  CostScalar numberOfRowsToBeInserted =
    context->getInputLogProp()->getResultCardinality();

  if (CIdesc->isPartitioned())
    {
      ValueIdSet tempNewRecExpr (bef->newRecExpr());

      // From the new record expression, divide predicates into
      // partitioning key predicates (on the primary index) vs. non
      // partitioning key predicates.
      ValueIdSet dummySet;
      skey = new(CmpCommon::statementHeap())
        SearchKey (CIdesc->getPartitioningKey(),
                   CIdesc->getOrderOfPartitioningKeyValues(),
                   externalInputs,
                   TRUE,
                   tempNewRecExpr,
                   dummySet,
                   CIdesc);

      // insert is always a row-at-a-time operator
      //CMPASSERT(skey->isUnique());
    }

  result = new(CmpCommon::statementHeap()) 
              HiveInsert(HiveInsert(bef->getTableName(),bef->getTableDesc()));

  copyCommonGenericUpdateFields(result, bef);

  result->setPartKey(skey);
  result->rrKeyExpr()              = bef->rrKeyExpr();
  result->rowPosInput()            = bef->rowPosInput();
  result->partNumInput()           = bef->partNumInput();
  result->totalNumPartsInput()     = bef->totalNumPartsInput();
  result->reqdOrder()              = bef->reqdOrder();
  result->setNoBeginCommitSTInsert(bef->noBeginSTInsert(), bef->noCommitSTInsert());
  result->enableTransformToSTI() = bef->enableTransformToSTI();

  if (result->getGroupAttr()->isEmbeddedInsert())
    result->executorPred() += bef->selectionPred();

  return result;
}

// -----------------------------------------------------------------------
// methods for class HbaseInsertRule
// -----------------------------------------------------------------------
HbaseInsertRule::~HbaseInsertRule() {} 

NABoolean HbaseInsertRule::topMatch(RelExpr * relExpr, Context *context)
{
  if (NOT Rule::topMatch(relExpr,context))
    return FALSE;

  CMPASSERT(relExpr->getOperatorType() == REL_LEAF_INSERT);
  Insert * ins= (Insert *) relExpr;

  if (ins->getTableDesc()->getNATable()->isHbaseTable() == FALSE)
    return FALSE;

  // HbaseInsert can only execute above DP2.
  if (context->getReqdPhysicalProperty()->executeInDP2())
    return FALSE;

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  //if (relExpr->rppRequiresEnforcer(context->getReqdPhysicalProperty()))
  //  return FALSE;

  return TRUE;
}

RelExpr * HbaseInsertRule::nextSubstitute(RelExpr * before,
                                              Context * context,
                                              RuleSubstituteMemory *& /*mem*/)
{
  CMPASSERT(before->getOperatorType() == REL_LEAF_INSERT);
  CMPASSERT(before->getArity() == 0);

  HbaseInsert * result;
  Insert * bef = (Insert *) before;

  // build the key for the insert from the expression for the new record
  SearchKey * skey = NULL;
  const IndexDesc * CIdesc = bef->getTableDesc()->getClusteringIndex();
  const PartitioningFunction* insPartFunc = CIdesc->getPartitioningFunction();

  ValueIdSet externalInputs = before->getGroupAttr()->getCharacteristicInputs();

   NABoolean rangeLoggingRequired =
    bef->getTableDesc()->getNATable()->getMvAttributeBitmap().getAutomaticRangeLoggingRequired();

  CostScalar numberOfRowsToBeInserted =
    context->getInputLogProp()->getResultCardinality();

  if (CIdesc->isPartitioned())
    {
      ValueIdSet tempNewRecExpr (bef->newRecExpr());

      // From the new record expression, divide predicates into
      // partitioning key predicates (on the primary index) vs. non
      // partitioning key predicates.
      ValueIdSet dummySet;
      skey = new(CmpCommon::statementHeap())
        SearchKey (CIdesc->getPartitioningKey(),
                   CIdesc->getOrderOfPartitioningKeyValues(),
                   externalInputs,
                   TRUE,
                   tempNewRecExpr,
                   dummySet,
                   CIdesc);

      // insert is always a row-at-a-time operator
      //CMPASSERT(skey->isUnique());
    }

  result = new(CmpCommon::statementHeap()) 
    HbaseInsert(bef->getTableName(),bef->getTableDesc());

  result->setInsertType(bef->getInsertType());

  DefaultToken vsbbTok = CmpCommon::getDefault(INSERT_VSBB);
  if ((numberOfRowsToBeInserted > 1) &&
      (vsbbTok != DF_OFF) &&
      (result->getInsertType() != Insert::UPSERT_LOAD))
    {
      result->setInsertType(Insert::VSBB_INSERT_USER);
    }

  copyCommonGenericUpdateFields(result, bef);

  result->setPartKey(skey);
  result->rrKeyExpr()              = bef->rrKeyExpr();
  result->rowPosInput()            = bef->rowPosInput();
  result->partNumInput()           = bef->partNumInput();
  result->totalNumPartsInput()     = bef->totalNumPartsInput();
  result->reqdOrder()              = bef->reqdOrder();
  result->setNoBeginCommitSTInsert(bef->noBeginSTInsert(), bef->noCommitSTInsert());
  result->enableTransformToSTI() = bef->enableTransformToSTI();
  result->setIsUpsert(bef->isUpsert());
  result->setIsTrafLoadPrep(bef->getIsTrafLoadPrep());
  result->setCreateUstatSample(bef->getCreateUstatSample());

  if (result->getGroupAttr()->isEmbeddedInsert())
    result->executorPred() += bef->selectionPred();

  return result;
}

// -----------------------------------------------------------------------
// methods for class NestedJoinRule
// -----------------------------------------------------------------------

NestedJoinRule::~NestedJoinRule() {} 

NABoolean NestedJoinRule::topMatch(RelExpr * relExpr,
                                   Context * context)
{

  if (NOT Rule::topMatch(relExpr,context))
    return FALSE;

  Join *joinExpr = (Join *) relExpr;

  // Only TSJ nodes are converted to Nested Joins.
  // Join nodes for which Nested Joins are possible should have
  // been converted to TSJs first.
  // So we want to make sure that if we have a RoutineJoin, which is a
  // type of TSJ, we don't want this rule to apply until we have converted
  // it to a TSJ, so return FALSE for RoutineJoins. isNonRoutineTSJ() returns
  // true for all TSJs except for RoutineJoins.
  if (NOT joinExpr->isNonRoutineTSJ()) return FALSE;

  // The avoidHalloweenR2 flag will be set on two Join nodes.  The join
  // used for the Subquery and the Tuple Flow Join used for write.  If
  // avoidHalloween is set and this is the join used for the subquery,
  // then it must use a hash join. So do not fire the nested join in
  // this case.  If this is the tuple flow for write, then this should fire.
  //
  if (joinExpr->avoidHalloweenR2() &&
      !joinExpr->isTSJForWrite())
    return FALSE;

  // nested join is not supported for Full_Outer_Join
  if (relExpr->getOperator().match(REL_ANY_FULL_JOIN))
    return FALSE;

  //++MV
  //If this node was marked in inlining for single execution ,the rule will be fired
  //only if there is a single input request.
  if (relExpr->getInliningInfo().isSingleExecutionForTSJ() &&
      !context->getInputLogProp()->isCardinalityEqOne())
  {
    return FALSE;
  }
  //--MV

  // QSTUFF
  // we can only implement embedded deletes if no join
  // is multiplying the number of delete tuples returned
  // this rejection hopefully causes the commutativity rule to trigger
  // right now is not really needed, only if we allow deletes on
  // as inner/right child.

  // if (joinExpr->getGroupAttr()->isEmbeddedUpdateOrDelete())
  //   if (NOT (joinExpr->isSemiJoin() OR
  //            joinExpr->isAntiSemiJoin() OR
  //            joinExpr->isCursorUpdate()))
  //     if (NOT joinExpr->rowsFromLeftHaveUniqueMatch())
  //       return FALSE;
  // QSTUFF

/*
  // ********************************************************************
  // This part is disabled until we have a better way of protecting
  // the normalizer output TSJs, and TSJs for write, and TSJs for
  // index joins. In other words, we should only do this heuristic
  // for "optional" TSJ's, i.e. those that were added by the
  // JoinToTSJRule.
  // ********************************************************************
  if (CURRSTMT_OPTDEFAULTS->optimizerHeuristic4() &&
      context->getInputLogProp()->getResultCardinality() <= 1 &&
      NOT relExpr->getOperator().match(REL_ANY_ANTI_SEMITSJ) &&
      NOT relExpr->getOperator().match(REL_ANY_SEMITSJ) )
     // logic of this heuristic does not apply for subqueries
     // also we will not prune semi joins cuz they might be normalizer
     // output (have no nontsj equivalent). Also we do not prune
     // anti-semi-join cuz so far this is its only implementation.

  {
    CostScalar myCardinality =
      relExpr->child(0).getGroupAttr()->getResultCardinalityForEmptyInput();
    //CostScalar cardinality0 =
      //relExpr->child(0).getGroupAttr()->getResultCardinalityForEmptyInput();
    CostScalar cardinality1 =
      relExpr->child(1).getGroupAttr()->getResultCardinalityForEmptyInput();

    if (myCardinality > cardinality1 AND cardinality1 > 10)
      return FALSE;
  }
*/
  const ReqdPhysicalProperty* const rppForMe =
                                context->getReqdPhysicalProperty();

  // ---------------------------------------------------------------
  // Only allow nested joins to be pushed down to dp2 if flag is on 
  // and the join node is allowed to be pushed down. Currently the
  // TSJRule sets the non-push-down flag to prevent the transformed
  // NestedJoin (for UNARY_INSERT) from being pushed down. 
  // 
  // Colocation check will be done later (need to improve here).
  // Currently, NJ in DP2 for write is disabled.
  // ---------------------------------------------------------------

  if (rppForMe->executeInDP2() AND 
      ( NOT CURRSTMT_OPTDEFAULTS->pushDownDP2Requested() OR
        joinExpr->allowPushDown() == FALSE ))
    return FALSE;

  // disallow pushdown of IM plans
  if (rppForMe->executeInDP2() AND
      relExpr->getInliningInfo().isDrivingIM())
    return FALSE;

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  if (relExpr->rppRequiresEnforcer(rppForMe))
    return FALSE;

  // If nested join is being forced for this operator then return TRUE now.
  if ((rppForMe->getMustMatch() != NULL) AND
      (rppForMe->getMustMatch()->getOperatorType() == REL_FORCE_NESTED_JOIN))
    return TRUE;

  // Fix genesis case 10-040524-2077 "NE:RG:mxcmp internal error when stream
  // used with rowset in WHERE clause" by commenting out the following code on
  // This code was intended to provide partial
  // support for identifying which element of a rowset caused an exception as
  // exemplified by regress/rowsets/teste140.sql's test10.
  // An Unpack node should never be the right child of a nested join as this would
  // cause the rownumber feature to  work incorrectly. We are enforcing
  // this condition below.
  //if ((joinExpr->isRowsetIterator()) &&
  //    (joinExpr->child(1).getGroupAttr()->getNumBaseTables() == 0) )
  //  return FALSE;

  // --------------------------------------------------------------------
  // Fire the rule only if NESTED_JOINS default is on, with the
  // following exceptions:
  // - If hash and merge joins are disabled or if hash and merge joins
  //   cannot work, we pick the nested join, even if it was disabled.
  //   A merge join will not work when the query does not have equi-join
  //   predicates.
  // ---------------------------------------------------------------------
  if (NOT CURRSTMT_OPTDEFAULTS->isHashJoinConsidered() AND
      (NOT CURRSTMT_OPTDEFAULTS->isMergeJoinConsidered() OR
       joinExpr->getEquiJoinPredicates().isEmpty()))
    return TRUE;
  else if ((CmpCommon::getDefault(COMP_BOOL_151) == DF_ON) &&
           (NOT CURRSTMT_OPTDEFAULTS->isNestedJoinConsidered())) {
    // allow mandatory nested joins even under nested_joins OFF
    // to fix genesis case 10-070215-6221, solution 10-070215-2604.
    // comp_bool_151 is a hedge if you want ALL nested_joins OFF.
    return FALSE;
  }

  // nested join is not good for filtering joins in the star join schema
  // This check already done in joinToTSJRule, but doublecheck again here
  if (CmpCommon::getDefault(COMP_BOOL_85) == DF_OFF &&
      (joinExpr->getSource() == Join::STAR_FILTER_JOIN) &&
      !(joinExpr->derivedFromRoutineJoin()))
    return FALSE;



  // ---------------------------------------------------------------------
  // Fire the NJ rule for reads that are not index joins and are not
  // for subqueries only if a join predicate exists or if there is a
  // required order or arrangement.  If a join predicate does not exist,
  // a hybrid hash join will perform better for building the cartesian
  // product, unless there is a required order, in which case a HHJ
  // won't work, because a HHJ cannot preserve order.
  // ---------------------------------------------------------------------
  // ********************************************************************
  //   We can't do this until the JoinToTSJRule passes us some indication
  // on whether the original pre-TSJ join was a cross product or not.
  // We cannot figure this out here because the join predicates have
  // already been pushed down. The ideal thing for the JoinToTSJRule
  // to do would be to stash the original join predicates in a new field
  // in the TSJ node. If we do this, we can also use the original join
  // predicates for other purposes, such as determining what the
  // equijoin columns are for doing Type-1 nested joins in DP2.
  //   Also, we need TSJ's added by the JoinToTSJRule to
  // be marked as "optional". For non-optional TSJ's, such as those
  // for write, for subqueries, or for index joins, we MUST always
  // fire the Nested Join rule because there is no other join method
  // available to them.
  //   SO, FOR NOW, ALWAYS SET isACrossProduct TO FALSE.
  // ********************************************************************

  // ---------------------------------------------------------------------
  // If hash joins are disabled consider doing a nested join even if
  // it is a cross product
  // ---------------------------------------------------------------------
  NABoolean isACrossProduct = FALSE;

  // Only consider NJ for cross products if there is a
  // order or arrangement requirement.Otherwise HHJ is better.
  // Now: HHJ can keep order
  if (isACrossProduct)
  {
    //if ((NOT rppForMe->getSortKey() OR
	//    (rppForMe->getSortKey()->entries() == 0)) AND

      if (NOT rppForMe->getArrangedCols() OR
         (rppForMe->getArrangedCols()->entries() == 0))
      return FALSE;
    // One other possible heuristic  we can have here is to require
    // that xproduct output is less than say 10% of largest table size
    // we can do this later
  }



  // MV --
  // If the Join was forced to some other type - fail it.
  if (joinExpr->isPhysicalJoinTypeForced() &&
      joinExpr->getForcedPhysicalJoinType() != Join::NESTED_JOIN_TYPE)
    return FALSE;


  return TRUE;

} // NestedJoinRule::topMatch

RelExpr * NestedJoinRule::nextSubstitute(RelExpr * before,
                                         Context * context, //*context*/,
                                         RuleSubstituteMemory *& /*memory*/)
{
  NestedJoin * result;
  Join     * bef = (Join *) before;

  // QSTUFF
  CMPASSERT (NOT bef->child(1).getGroupAttr()->isStream());
  //  CMPASSERT (NOT bef->child(1).getGroupAttr()->isEmbeddedUpdateOrDelete());
  // QSTUFF


  // build the result from the join node in the before pattern
  result = new(CmpCommon::statementHeap())
    NestedJoin(bef->child(0),
               bef->child(1),
               bef->getNestedJoinOpType(),
               CmpCommon::statementHeap(),
               bef->updateTableDesc(),
               bef->updateSelectValueIdMap());

  // now set the group attributes of the result's top node
  result->setGroupAttr(before->getGroupAttr());

  // transfer all the join fields to the new NestedJoin
  (void) bef->copyTopNode(result,CmpCommon::statementHeap());

  // now we need to make sure that the order and arrangement requirement
  // , if any, can be passed to the NJ childeren
  const ReqdPhysicalProperty* const rppForMe =
                                context->getReqdPhysicalProperty();

  ValueIdList dummy1, dummy2;
  if (rppForMe->getSortKey() AND
      rppForMe->getSortKey()->entries() > 0 AND
      NOT result->splitOrderReq(*(rppForMe->getSortKey()),dummy1,dummy2))
    return NULL;

  ValueIdSet dummy3, dummy4;
  if (rppForMe->getArrangedCols() AND
      rppForMe->getArrangedCols()->entries() > 0 AND
      NOT result->splitArrangementReq(*(rppForMe->getArrangedCols()),dummy3,dummy4))
    return NULL;

  // Pass the original equal join expression to the physical nest join
  // operator so that OCR can use it to check if the equi join expression
  // completes covers right child's partitioning key.
  result->setOriginalEquiJoinExpressions(bef->getOriginalEquiJoinExpressions());

  return result;

} // NestedJoinRule::nextSubstitute()

NABoolean NestedJoinRule::canBePruned(RelExpr * relExpr) const
{
  // We do not want to prune Nested Join Rule if expr is an AntiSemiJoin
  // since thats its only possible implementation in many cases
  // *************************************************************
  // What about Nested Join for write ops? Or subqueries? Or
  // index joins? For all of these, Nested Join is the only
  // possible implementation. Shouldn't we not prune NJ for them?
  // We should only prune NJ for TSJ's that are marked as "optional",
  // i.e. were added by the JoinToTSJRule.
  // HOW DOES THIS POSSIBLY WORK? IT SEEMS THAT PRUNING CAN PRUNE
  // MANDATORY NJ's AND SOME PLANS SHOULD FAIL WITH PRUNING ON. ???
  // *************************************************************
  return NOT relExpr->getOperator().match(REL_ANY_ANTI_SEMIJOIN);
}

// -----------------------------------------------------------------------
// methods for class NestedJoinFlowRule
// -----------------------------------------------------------------------

NestedJoinFlowRule::~NestedJoinFlowRule() {} 

NABoolean NestedJoinFlowRule::topMatch (RelExpr *relExpr,
                                        Context *context)
{
  if (NOT Rule::topMatch(relExpr, context))
    return FALSE;

  Join * joinExpr = (Join *) relExpr; // QSTUFF ?

  // QSTUFF
  // we can only implement embedded deletes if no join
  // is multiplying the number of delete tuples returned
  // this rejection hopefully causes the commutativity rule to trigger

  // if (joinExpr->getGroupAttr()->isEmbeddedUpdateOrDelete() &&
  //      !joinExpr->rowsFromLeftHaveUniqueMatch()){
  //      return FALSE;
  // }
  // QSTUFF

  // if execution inside DP2 is intended, check a few more things.
  // NSJ is allowed to run inside DP2 only if it is inside a CS.
  if (context->getReqdPhysicalProperty()->executeInDP2())
  {
     // if there is no request for pushing down, return false.
     if ( NOT CURRSTMT_OPTDEFAULTS->pushDownDP2Requested() )
        return FALSE;
  }

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  if (relExpr->rppRequiresEnforcer(context->getReqdPhysicalProperty()))
    return FALSE;

  return TRUE;
}

RelExpr * NestedJoinFlowRule::nextSubstitute(RelExpr * before,
                                             Context * /*context*/,
                                             RuleSubstituteMemory *& /*memory*/)
{
  NestedJoinFlow * result;
  Join     * bef = (Join *) before;

  // QSTUFF
  CMPASSERT (NOT bef->child(1).getGroupAttr()->isStream());
  CMPASSERT (NOT bef->child(1).getGroupAttr()->isEmbeddedUpdateOrDelete());
  // QSTUFF

  // A tuple substitution flow operator must not have any predicates.
  CMPASSERT(bef->nullInstantiatedOutput().entries() == 0);
  CMPASSERT(bef->selectionPred().entries() == 0);
  CMPASSERT(bef->joinPred().entries() == 0);

  // build the result from the join node in the before pattern
  result = new(CmpCommon::statementHeap())
    NestedJoinFlow(bef->child(0),
                   bef->child(1),
                   bef->updateTableDesc(),
                   bef->updateSelectValueIdMap());


  // now set the group attributes of the result's top node
  result->setGroupAttr(before->getGroupAttr());

  result->setBlockStmt(before->isinBlockStmt());

 // transfer all the join fields to the new NestedJoinFlow
  (void) bef->copyTopNode(result,CmpCommon::statementHeap());

  // The following code adds clustering index into GA's availableBtreeIndexes.
  // The logic is no longer needed because we can not use the availableBtreeIndexes
  // to check the partition func to the inner target table is compatible with the
  // table, because the part keys of the table is never exposed in its char. output.

  //GroupAttributes* ga = bef->child(1)->getGroupAttr();
  //RelExpr * r = bef->child(1)->castToRelExpr();
  //if ( r->getOperatorType() == REL_UNARY_INSERT ) {
  //   
  //   Insert* insNode = (Insert*)r;
  //
  //    if ( insNode->getTableDesc()->getNATable()->isHiveTable() ) {
  //      SET(IndexDesc *) x;
  //     x.insert((IndexDesc*)(insNode->getTableDesc()->getClusteringIndex()));
  //      ga->addToAvailableBtreeIndexes(x);
  //   }
  //}

  return result;

} // NestedJoinFlowRule::nextSubstitute()

// -----------------------------------------------------------------------
// methods for class HashJoinRule
// -----------------------------------------------------------------------

HashJoinRule::~HashJoinRule() {} 

NABoolean HashJoinRule::topMatch (RelExpr * relExpr,
                                  Context * context)
{
  if (NOT Rule::topMatch(relExpr,context))
    return FALSE;

  Join * joinExpr = (Join *) relExpr;

  // Execute the hash join transformation iff this is not a tsj.
  if ( joinExpr->isTSJ() )
    return FALSE;

  // -----------------------------------------------------------------
  // Hybrid hash join cannot preserve order,
  // if it is not implementing a cross product.
  // We will try to choose a cross-product if the join predicate and
  // selection predicate are empty. So if the context requires order
  // no longer assume that OHJ is the only operator that can provide
  // that order. If the join & selection predicates are empty
  // a cross product can provide the order required by the context.
  // -----------------------------------------------------------------
  NABoolean contextRequiresOnlyLeftOrder = (context->requiresOrder() AND
		  (NOT (joinExpr->isInnerNonSemiJoinWithNoPredicates())));
  if (contextRequiresOnlyLeftOrder AND
      (NOT CURRSTMT_OPTDEFAULTS->isOrderedHashJoinConsidered()))
	return FALSE;

  // Don't allow hash joins if this is not the first pass and
  // hash joins have been disabled.
  if(NOT CURRSTMT_OPTDEFAULTS->isHashJoinConsidered() AND
     // We no longer need to protect hash join from being turned off for pass 1
     // because now we allow nested join in pass 1 too
     ((CmpCommon::getDefault(COMP_BOOL_81) == DF_OFF) OR
      (GlobalRuleSet->getCurrentPassNumber() >
       GlobalRuleSet->getFirstPassNumber())))
    return FALSE;

  // QSTUFF
  // can't yet join two streams or
  // have right child being a stream
  if (joinExpr->getGroupAttr()->isStream())
    return FALSE;

  // Don't want to hash joins and GET_NEXT_N commands yet
  if (context->getGroupAttr()->isEmbeddedUpdateOrDelete())
    return FALSE;
  // QSTUFF

  const ReqdPhysicalProperty* const rppForMe =
                                context->getReqdPhysicalProperty();

  // -----------------------------------------------------------------
  // Hybrid hash joins cannot be done in dp2.
  // -----------------------------------------------------------------
  if (rppForMe->executeInDP2())
    return FALSE;

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  if (relExpr->rppRequiresEnforcer(rppForMe))
    return FALSE;

  // MV --
  // If the Join was forced to some other type - fail it.
  if (joinExpr->isPhysicalJoinTypeForced() &&
      joinExpr->getForcedPhysicalJoinType() != Join::HASH_JOIN_TYPE)
    return FALSE;

  // don't consider hash joins if we don't expect to have more
  // than one row in the left child table
  // But don't apply this rule is the join is configured to 
  // avoid halloween (for Neo R2 compatibility).

  // Full Outer Join is supported ONLY by Hash join, so let's
  // not eliminate our ONLY option.
  // Moreover, COMP_BOOL_196 guard is there because,
  // we want to be able to reproduce the scenario, where,
  // when direct Pass 2 optimization fails to produce
  // a plan, the engine goes backs and tries Pass 1.
  // Currently this (retrying Pass 1) does not happen.
  // When that problem is fixed, the COMP_BOOL_196 guard
  // must be removed.

  if (joinExpr->allowHashJoin() == FALSE AND context AND
      context->getReqdPhysicalProperty()->getMustMatch() == NULL)
    return FALSE;

  if (contextRequiresOnlyLeftOrder)
  {
    // Heuristically eliminate less promising ordered hash joins.
    if (CURRSTMT_OPTDEFAULTS->isOrderedHashJoinControlEnabled() AND
        (CmpCommon::getDefault(COMP_BOOL_31) == DF_OFF) AND
        context->getInputLogProp()->getResultCardinality() <= 1)
      {
        // innerS is HIGH bound size estimate of inner table in OHJ plan.
        // This max cardinality based innerS should hopefully allow us 
        // to safely use CQD HJ_TYPE 'SYSTEM' as the default setting.
        GroupAttributes * innerGA = relExpr->child(1).getGroupAttr();
        CostScalar innerS = innerGA->getResultMaxCardinalityForEmptyInput() 
          * innerGA->getCharacteristicOutputs().getRowLength();

        // avoid OHJ if inner table will not fit into memory
        if (innerS > CostScalar(1024* CURRSTMT_OPTDEFAULTS->getMemoryLimitPerCPU()))
          return FALSE;
      }
  }

  return TRUE;
}

// internal routine, used only by HashJoinRule::nextSubstitute() to set
// the flags in the "result" before returning.
static RelExpr * setResult(HashJoin * result,
			   Join     * bef,
			   NABoolean isLeftOrdered,
			   NABoolean isOrderedCrossProduct)
{
  // for ordred cross products order cannot be be promised
  // unless setNoOverflow flag is set. i.e in case
  // there is overflow we want to raise error rather than
  // handle the overflow.
  result->setNoOverflow(isLeftOrdered || isOrderedCrossProduct);
  result->setIsOrderedCrossProduct(isOrderedCrossProduct) ;

  if (NOT (isLeftOrdered))
    result->setReuse(FALSE); // HYBRID CANNOT DO REUSE,

  // Don't reuse the cluster for Full Outer Join (FOJ), till
  // the executor support, FOJ with cluster reuse.

  if(bef->getOperatorType() == REL_FULL_JOIN)
     result->setReuse(FALSE);

  // Don't reuse when under an AFTER ROW trigger !
  // This avoids the problem of reusing the trigger temporary table while
  // also inserting into that TTT in the same plan (i.e., the reused hash-table
  // does not have the newly inserted rows.)
  if (CURRSTMT_OPTDEFAULTS->areTriggersPresent())
    result->setReuse(FALSE);

  if (isLeftOrdered)
    result->setOperatorType( bef->getHashJoinOpType(isLeftOrdered));

  return result;
}

RelExpr * HashJoinRule::nextSubstitute(RelExpr * before,
                                       Context * context,
                                       RuleSubstituteMemory *& /*memory*/)
{
  Join       * bef = (Join *) before;

  // Atmost only one of these two flags will be true.
  // if context->requiresOrder() is FALSE then requiresOnlyLeftOrder = FALSE
  // and orderedCrossProduct may be TRUE.
  // if context->requiresOrder() is TRUE then if the ordering
  // requirement can be satisfied by the left child alone we will
  // try to produce an OHJ plan, otherwise if conditions
  // for orderedCrossProduct are satisfied (no predicates etc.)
  // we will try to produce an Ordered-cross-product (OCP) plan,
  // by checking if the ordering requirements can be split.
  // If this fails too we return NULL as the ordering requirement
  // cannot be satisfied by OHJ or OCP. In summary the block of
  // code below decides if the order required by the context can be
  // satisfied by an OHJ plan or an OCP plan (in that order)
  NABoolean requiresOnlyLeftOrder = FALSE;
  NABoolean orderedCrossProduct = FALSE;
  const ReqdPhysicalProperty* const rppForMe =
			    context->getReqdPhysicalProperty();

  // if context requires order then we try an OHJ first
  if (context->requiresOrder() AND
      (CURRSTMT_OPTDEFAULTS->isOrderedHashJoinConsidered()))
  {
    // Can Order or arrangement requirements be satisfied by the left
    // child alone?

    ValueIdSet orderArrangementExpr, newInputs,
      coveredExpr, referencedInputs;

    if (rppForMe->getSortKey())
      orderArrangementExpr.insertList(*rppForMe->getSortKey());
    if (rppForMe->getArrangedCols())
      orderArrangementExpr += *rppForMe->getArrangedCols();

    bef->child(0).getGroupAttr()->coverTest(orderArrangementExpr,
						newInputs,
						coveredExpr,
						referencedInputs);

    if (coveredExpr == orderArrangementExpr)
    {
      // order required by context can be satisfied by left child alone
      requiresOnlyLeftOrder = TRUE;
    }
  }

  // Join::isInnerNonSemiJoinwithNoPredicates() is a stricter version
  // of Join::isCrossProduct(). This method returns TRUE if
  // join predicate is empty AND selectionPred is  empty and
  // the join is not (a semijoin, or anti join or outer join).
  // We use this method instead of isCrossProduct() because
  // isCrossProduct will return TRUE if the Selection/Join predicates
  // have something other than a VEG predicate. The one we saw was
  // a > predicate.
  if ((NOT requiresOnlyLeftOrder) AND
      bef->isInnerNonSemiJoinWithNoPredicates() AND
      bef->getSource() == Join::STAR_KEY_JOIN AND
      CmpCommon::getDefault(COMP_BOOL_73) == DF_OFF)
  {
      // satisfies the basic conditions of being
      // an order-preserving-cross-product
      // right now OCP plans are enabled only for
      // joining dimension tables for a StarJoin rule.
      orderedCrossProduct = TRUE;

      // If the HJ is implementing a cross-product and the context
      // requires order then check to see if the required order and
      // arrangement can be split among the children.
      if (context->requiresOrder())
      {
	// for a cross-product whose context requires order return FALSE if the
	// ordering requirement cannot be split among the children.
	ValueIdList dummy1, dummy2;
	ValueIdSet dummy3, dummy4;
	if ((rppForMe->getSortKey() AND
	    rppForMe->getSortKey()->entries() > 0 AND
	    NOT bef->splitOrderReq(*(rppForMe->getSortKey()),dummy1,dummy2))
	OR
	    (rppForMe->getArrangedCols() AND
	    rppForMe->getArrangedCols()->entries() > 0 AND
	    NOT bef->splitArrangementReq(*(rppForMe->getArrangedCols()),dummy3,dummy4)))
	{
	  // order required by context cannot be satisfied by OHJ or OCP
	  return NULL;
	}
      }
  }

  // if not OHJ and not OCP and context requires order
  // then no HJ node can satisfy the order req.
  if (context->requiresOrder() AND
      (NOT orderedCrossProduct) AND
      (NOT requiresOnlyLeftOrder))
	return NULL;

  Int32 multipleCallsToChild = FALSE;


  // Build the result tree,
  // temporarily assume a hybrid hash join when choosing an operator type
  HashJoin   * result = new(CmpCommon::statementHeap())
    HashJoin(bef->child(0),
	     bef->child(1),
	     bef->getHashJoinOpType(FALSE));

  result->setGroupAttr(before->getGroupAttr());

  // transfer all the join fields to the new HashJoin
  (void) bef->Join::copyTopNode(result,CmpCommon::statementHeap());

  result->resolveSingleColNotInPredicate();

  // input log prop for the node
  EstLogPropSharedPtr inLogProp = context->getInputLogProp();

  NABoolean joinStrategyIsOrderSensitive = FALSE;
  result->separateEquiAndNonEquiJoinPredicates(joinStrategyIsOrderSensitive);

  // Get addressability to the defaults table.
  NADefaults &defs = ActiveSchemaDB()->getDefaults();
  // OHJ_BMO_REUSE_SORTED_UECRATIO_UPPERLIMIT = 0.70
  double BMOReuseSortedUECRatioUpperlimit =
  defs.getAsDouble(OHJ_BMO_REUSE_SORTED_UECRATIO_UPPERLIMIT);

  // number of parent probes
  CostScalar probeCount =
    MAXOF(1.,inLogProp->getResultCardinality().value());

  // ---------------------------------------------------------------------
  // Determine REUSE possibility.
  // Reuse may be a good idea if this hash join gets invoked multiple
  // times and if the data returned from the right child fits in memory.
  // ---------------------------------------------------------------------
  if ( inLogProp->getResultCardinality() > 1 )
    {
      // We may declare a REUSE here as a possiblity. However, later, during
      // costing, IFF we find out that REUSE is not very useful,
      // because the calls to the inner table are too numerous and the inputs
      // are NOT SORTED, we will OVERTURN this decision and go with HYBRID AND
      // NO REUSE. Since we do not have the information on whether the inputs
      // are SORTED, we cannot make that decision here.

      // check whether the right child is dependent on the
      // characteristic inputs of this join (set multipleCallsToChild to
      // true if so)
      EstLogPropSharedPtr inputLogPropForChild = result->child(1).getGroupAttr()->
	materializeInputLogProp(
	     context->getInputLogProp(),&multipleCallsToChild);
      result->multipleCalls()=multipleCallsToChild;

      if(NOT multipleCallsToChild)
	result->setReuse(TRUE); // Set this to be a REUSE Candidate

      // if, however, the inner table will be called multiple times, make sure
      // that these calls are not so numerous that reuse becomes too expensive
      // ********how to find order of the char inputs
      else
	{
	  // Characteristic inputs are the values whose change causes
	  // reinitialization of the table.
	  ValueIdSet tochild =
	    result->child(1).getGroupAttr()->getCharacteristicInputs();
	  result->valuesGivenToChild() = tochild;

	  // Unique entry Count for the parent probes
	  CostScalar uniqueProbeCount =
	    inLogProp->getAggregateUec(result->valuesGivenToChild());

	  CostScalar uniqueRatio = uniqueProbeCount/probeCount;

	  if(uniqueRatio <= BMOReuseSortedUECRatioUpperlimit)
	    result->setReuse(TRUE);
	}
    } // end resultCardinality > 1

  // ---------------------------------------------------------------------
  // The following code compares hybrid hash join and ordered hash join
  // and picks the better one. It tries to handle the easy cases first.
  //
  // An ordered hash join should only be used if there is a reason for
  // it, because of the added risk of thrashing if the in-memory table
  // gets too large. There are two reasons for an ordered hash join:
  // a) an order is required and/or b) the reuse feature of the ordered
  // hash join is desired
  // ---------------------------------------------------------------------

  // ordered hash joins are treated like a materialize node with respect
  // to triggers, disable them when materialize nodes are forbidden
  if (CURRSTMT_OPTDEFAULTS->areTriggersPresent())
    if (requiresOnlyLeftOrder)
      return NULL;
    else
    {
      // no cross-product ordering can be promised with triggers
      return setResult(result,bef,FALSE,FALSE);
    }


  // Check if CQS is in force (it will override CQDs and other considerations)
  const RelExpr * mm = context->getReqdPhysicalProperty()->getMustMatch() ;
  if (mm)
    {
      switch (mm->getOperatorType())
	{
	case REL_FORCE_ORDERED_HASH_JOIN:
	  return setResult(result,bef,TRUE, FALSE);
	case REL_FORCE_HYBRID_HASH_JOIN:
	  return setResult(result,bef,FALSE, FALSE);
	case REL_FORCE_ORDERED_CROSS_PRODUCT:
	  return setResult(result,bef,FALSE, TRUE); 
	default:
	  break;
	}
    }

  // deal with the HJ_TYPE CQD if not both join types are enabled
  if (NOT (CURRSTMT_OPTDEFAULTS->isOrderedHashJoinConsidered() AND
           CURRSTMT_OPTDEFAULTS->isHybridHashJoinConsidered()))
    {
      if (CURRSTMT_OPTDEFAULTS->isOrderedHashJoinConsidered())
	// user wants only ordered hash joins
	return setResult(result,bef,TRUE, FALSE);
      else if (CURRSTMT_OPTDEFAULTS->isHybridHashJoinConsidered())
	if (context->requiresOrder() AND (NOT orderedCrossProduct))
	  // user wants hybrid joins, which can't be used here
	  return NULL;
	else
          // user wants hybrid hash joins, disable reuse if needed
	  return setResult(result,bef,FALSE, orderedCrossProduct);
    }

  // use a hybrid hash join if neither reason a) nor b) above applies
  if (NOT result->isReuse() AND
      NOT requiresOnlyLeftOrder)
    return setResult(result,bef,FALSE,orderedCrossProduct);

  // bmoFactor tells how big is it? =inner-tab_size/mem_size
  double bmoFactor = 0;
  double vBMOlimit = defs.getAsDouble(OHJ_VBMOLIMIT); // 5.0
  double bmoMemLimit = defs.getAsDouble(BMO_MEMORY_SIZE); // 200Mb

  // ---------------------------------------------------------------------
  // Now we have a case where we would like to reuse or benefit from
  // the order of the ordered hash join. Determine based on heuristics
  // what is better: Ordered hash join (with reuse), not to generate
  // any plan at all, or a hybrid hash join w/o reuse. In short, the
  // choices are reuse, refuse, or refuse to reuse ;-)
  // ---------------------------------------------------------------------
  // OHJ fixes
  // 1) If the memoryLimit_>200MB then temorarily set memoryLimit_ to 
  //    200MB. The idea is not to choose OHJ if the inner table size is
  //    > 200MB per ESP.
  if (CmpCommon::getDefault(COMP_BOOL_37) == DF_OFF) {
    double memoryLimitPerCPU = CURRSTMT_OPTDEFAULTS->getMemoryLimitPerCPU();
    if (memoryLimitPerCPU > bmoMemLimit)  // memory limit is in KB.
      // Temporarily set memory limit to 200MB.
      // BMO_MEMORY_SIZE can be used to control this 200MB limit.
      CURRSTMT_OPTDEFAULTS->setMemoryLimitPerCPU(bmoMemLimit);
    // Check if this is a BMO
    NABoolean isBMO = result->isBigMemoryOperatorSetRatio(context, 0, bmoFactor);
    // Reset memoryLimit
    CURRSTMT_OPTDEFAULTS->setMemoryLimitPerCPU(memoryLimitPerCPU);
    if (isBMO ) 
      if (requiresOnlyLeftOrder)
        // can't preserve order for this large hash join
        return NULL;
      else 
        // sacrifice reuse and do a hybrid hash join
        // order cannot be promised for orderedCrossProducts
        return setResult(result,bef,FALSE, FALSE);
    else  // Not a BMO => Great case for ordered hash join
      if (orderedCrossProduct)
        return setResult(result,bef,FALSE, TRUE);
      else
        return setResult(result,bef,TRUE, FALSE);
  }


  // The following block of code is old and contains bugs.
  // Mask the block out for code coverage.

  // Check if this is a BMO
  NABoolean isBMO = result->isBigMemoryOperatorSetRatio(context, 0, bmoFactor);
  NABoolean useOrderedHashJoin = FALSE;

  // Not a BMO => Great case for ordered hash join
  if (NOT isBMO) {
    if (orderedCrossProduct)
      return setResult(result,bef,FALSE, TRUE);
    else
      return setResult(result,bef,TRUE, FALSE);
  }

  // If the ratio > OHJ_VBMOLIMIT, the join is very big mem operator and there
  // is no use trying ordered hash join because of pagefaults.

  if (isBMO AND bmoFactor > vBMOlimit)
    {
      if (requiresOnlyLeftOrder)
	// can't preserve order for this large hash join
	return NULL;
      else
	// sacrifice reuse and do a hybrid hash join
	// (this should be a rare event, it's expensive to perform
	// a large hash join several times over)
	return setResult(result,bef,FALSE, FALSE); // order cannot be promised for orderedCrossProducts
    }

  // Heuristics to decide between Ordered and Hybrid when
  // BMO and REUSE and operator size is between memoryLimit
  // and VBMOFACTOR*memoryLimit. In this range be pessimistic and
  // do not make any promises about whether a crossproduct will
  // be ordered.

  // If the inner table is to be built only once, do Left-Ordered
  // (typically with Reuse).
  if(NOT multipleCallsToChild)
    return setResult(result,bef,TRUE, FALSE);

  // Unique entry Count for the parent probes
  CostScalar uniqueProbeCount =
    inLogProp->getAggregateUec (result->valuesGivenToChild());
  CostScalar UECRatio = uniqueProbeCount / probeCount;

  // There is no easy way to determine if the input probes from the parent
  // are sorted. So, for now, assume they are not sorted. This is pessimistic
  // approach and favors selection of hybrid_hash over ordered_hash.
  // The heuristics below use these defaults:
  // OHJ_BMO_REUSE_SORTED_BMOFACTOR_LIMIT = 3.0
  // OHJ_BMO_REUSE_UNSORTED_UECRATIO_UPPERLIMIT = 0.01
  if( context->getPlan() &&
      context->getPlan()->getPhysicalProperty() &&
      context->getPlan()->getPhysicalProperty()->isSorted() ) //input is sorted
    {
      double BMOReuseSortedBmofactorLimit =
	defs.getAsDouble(OHJ_BMO_REUSE_SORTED_BMOFACTOR_LIMIT);

      if( bmoFactor <= BMOReuseSortedBmofactorLimit) {

	useOrderedHashJoin = UECRatio <= BMOReuseSortedUECRatioUpperlimit ;
      }
      else  // if bmoFactor > OHJ_BMO_REUSE_SORTED_BMOFACTOR_LIMIT
	useOrderedHashJoin = UECRatio <= BMOReuseSortedUECRatioUpperlimit / 3;
    }
  else    // input is not sorted
    {
      double BMOReuseUnsortedUECRatioUpperlimit =
      	defs.getAsDouble(OHJ_BMO_REUSE_UNSORTED_UECRATIO_UPPERLIMIT) ;

      useOrderedHashJoin = UECRatio <= BMOReuseUnsortedUECRatioUpperlimit ;
    }

  return setResult(result,bef,useOrderedHashJoin, FALSE);
  //end Heuristics


} // HashJoinRule::nextSubstitute()

// -----------------------------------------------------------------------
// methods for class MergeJoinRule
// -----------------------------------------------------------------------

MergeJoinRule::~MergeJoinRule() {} 

NABoolean MergeJoinRule::topMatch (RelExpr *relExpr,
                                   Context * context)
{
  if (NOT Rule::topMatch(relExpr, context))
    return FALSE;

  Join * joinExpr = (Join *) relExpr;

  // Fire the merge join rule iff this is not a tsj nor a part of a
  // compound statement.
  if ( joinExpr->isTSJ() || relExpr->isinBlockStmt() )
    return FALSE;

  // avoid merge join on seabase MD tables
  if (joinExpr->getGroupAttr()->isSeabaseMDTable())
    return FALSE;

  // The avoidHalloweenR2 flag will be set on two Join nodes.  The join
  // used for the Subquery and the Tuple Flow Join used for write.  If
  // avoidHalloweenR2 is set and this is the join used for the subquery,
  // then it must use a hash join. So do not fire the merge join in
  // either case.
  //
  if (joinExpr->avoidHalloweenR2())
    return FALSE;

  // merge join is not supported for Full_Outer_Join
  if (relExpr->getOperator().match(REL_ANY_FULL_JOIN))
    return FALSE;

  // QSTUFF
  // can't do a merge join with streams
  // as we are missing end-of-data to kick of a sort
  if (joinExpr->getGroupAttr()->isStream())
    return FALSE;

  // Don't want to tangle merge joins and GET_NEXT_N commands yet
  if (context->getGroupAttr()->isEmbeddedUpdateOrDelete())
    return FALSE;
  // QSTUFF

  // Allow merge joins
  if (NOT CURRSTMT_OPTDEFAULTS->isMergeJoinConsidered())
    return FALSE;

  const ReqdPhysicalProperty * rppForMe = context->getReqdPhysicalProperty();

  // disallow merge join in dp2
  if (rppForMe->executeInDP2())
    return FALSE;

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  if (relExpr->rppRequiresEnforcer(rppForMe))
    return FALSE;

  // MV --
  // If the Join was forced to some other type - fail it.
  if (joinExpr->isPhysicalJoinTypeForced() &&
      joinExpr->getForcedPhysicalJoinType() != Join::MERGE_JOIN_TYPE)
    return FALSE;

  // ---------- Merge Join Control Heuristic --------------------------
  NABoolean checkForceMJ = 
    ((rppForMe->getMustMatch() != NULL) AND
     (rppForMe->getMustMatch()->getOperatorType() == REL_FORCE_MERGE_JOIN));
  if (!checkForceMJ &&
      (CURRSTMT_OPTDEFAULTS->isMergeJoinControlEnabled() ||
       CmpCommon::getDefault(COMP_BOOL_104) == DF_ON) &&
      (CURRSTMT_OPTDEFAULTS->isNestedJoinConsidered() ||
       CURRSTMT_OPTDEFAULTS->isHashJoinConsidered())
      )
  {
    CostScalar r0 =
      relExpr->child(0).getGroupAttr()->getResultCardinalityForEmptyInput()
      * relExpr->child(0).getGroupAttr()->getRecordLength();

    CostScalar r1 =
      relExpr->child(1).getGroupAttr()->getResultCardinalityForEmptyInput()
      * relExpr->child(1).getGroupAttr()->getRecordLength();

      if ( (r1 < CostScalar(1024* CURRSTMT_OPTDEFAULTS->getMemoryLimitPerCPU()))  OR
           ((CURRSTMT_OPTDEFAULTS->isMergeJoinControlEnabled()) &&
            (r0 < CostScalar(1024* CURRSTMT_OPTDEFAULTS->getMemoryLimitPerCPU()))))
        return FALSE;  //Hash is better ??unless zigzag off then one case void
  }
  // -------------------------------------------------------------------

  if (CmpCommon::getDefault(MERGE_JOIN_ACCEPT_MULTIPLE_NJ_PROBES) == DF_OFF && 
      context->getInputLogProp()->getResultCardinality() > 1)
    return FALSE;

  // Consider the merge join only if there's an equi-join predicate
  if ( joinExpr->getEquiJoinPredicates().isEmpty() )
    return FALSE;

  // consider merge join only if its join predicate has no constants.
  // otherwise, we may get the problem reported in 
  // genesis case 10-081117-8475 soln 10-081117-7342
  ItemExpr *constant = NULL;
  return (NOT (joinExpr->getJoinPred().referencesAConstValue(&constant)));
}

RelExpr * MergeJoinRule::nextSubstitute(RelExpr * before,
                                        Context * context,
                                        RuleSubstituteMemory *& /*memory*/)
{
  MergeJoin * result;
  Join      * bef = (Join *) before;

  result = new (CmpCommon::statementHeap())
               MergeJoin (bef->child(0),
                          bef->child(1),
                          bef->getMergeJoinOpType());

  // Set the group attributes of the result's top node
  result->setGroupAttr (bef->getGroupAttr());

  // transfer all the join fields to the new MergeJoin
  (void) bef->copyTopNode(result,CmpCommon::statementHeap());
  
  result->resolveSingleColNotInPredicate();

  NABoolean joinStrategyIsOrderSensitive = TRUE;
  result->separateEquiAndNonEquiJoinPredicates(joinStrategyIsOrderSensitive);

  // We must have at least 1 equijoin predicate and the partitioning
  // requirement should be a compatible one.
  if ( (NOT result->getEquiJoinPredicates().isEmpty()) AND
      (result->parentAndChildPartReqsCompatible
                 (context->getReqdPhysicalProperty())))
    return result;
  else
    return NULL;

}

NABoolean MergeJoinRule::canBePruned(RelExpr * relExpr) const
{
  // We do not want to prune Merge Join Rule if expr is an AntiSemiJoin.
  // Since Hash Join is not possible for ASJ and so MJ may be the only
  // good alternative.
  return NOT relExpr->getOperator().match(REL_ANY_ANTI_SEMIJOIN);
}

// -----------------------------------------------------------------------
// methods for class PhysCompoundStmtRule
// -----------------------------------------------------------------------

NABoolean PhysCompoundStmtRule::topMatch(RelExpr *relExpr,
                                      Context *context)
{
  if (relExpr->getOperatorType() != REL_COMPOUND_STMT)
    return FALSE;

  if (NOT Rule::topMatch(relExpr, context))
    return FALSE;

   // QSTUFF
  CMPASSERT(NOT(relExpr->getGroupAttr()->isStream() OR
                relExpr->getGroupAttr()->isEmbeddedUpdateOrDelete()));
  // QSTUFF

  const ReqdPhysicalProperty * rppForMe = context->getReqdPhysicalProperty();

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  if (relExpr->rppRequiresEnforcer(rppForMe))
    return FALSE;

  if ( CURRSTMT_OPTDEFAULTS->pushDownDP2Requested() )
  {
    // If this CS is to be pushed down and its total input/output
    // length is large than 36KB, we reject the request.
    //
    // 36KB is the upper limit of the space allocated
    // for inputs/outputs to DP2, and the summary of the total sizes of
    // of all types involved should be close to the final
    // SQLARK_EXPLODED_FORMAT size. Here we use a fudge factor of 4K.
    //
    // It is found that DP2 goes into infinite loop if we have rowsize
    // of approximately > 30000. Internally the executor has a max buffer
    // size of 31000 for nodes with remotepartition and 56000 for local
    // partitions.30000 bytes limit is experimentally found to be safe after
    // taking into consideration the additional bytes that will be added for
    // header information and control information in executor.
    //

    // Using the CQD to increase this limit to 55000 for the DBlimits project
    // to support large rows.

    if ( rppForMe->executeInDP2() == TRUE ) {

      Int32 limit =
          (CmpCommon::getDefault(GEN_DBLIMITS_LARGER_BUFSIZE) == DF_OFF) ?
          ROWSIZE_TO_EXECUTE_IN_DP2 : ROWSIZE_TO_EXECUTE_IN_DP2_DBL;

      if ( relExpr->getGroupAttr()->getCharacteristicInputs().getRowLength() +
           relExpr->getGroupAttr()->getCharacteristicOutputs().getRowLength()
          > limit )
        return FALSE;

    } else {
      if (rppForMe->getPlanExecutionLocation() != EXECUTE_IN_MASTER)
        return FALSE;
    }
  }
  else {
    if (rppForMe->getPlanExecutionLocation() != EXECUTE_IN_MASTER)
      return FALSE;
  }

  return TRUE;
} // PhysCompoundStmtRule::topMatch

RelExpr *PhysCompoundStmtRule::nextSubstitute(
                             RelExpr *before,
                             Context *context,
                             RuleSubstituteMemory * & memory)
{
  RelExpr *result;

  CMPASSERT(before->getOperatorType() == REL_COMPOUND_STMT);

  result = new (CmpCommon::statementHeap())
    PhysCompoundStmt(*((CompoundStmt *)before));

  result->setGroupAttr(before->getGroupAttr());

  result->allocateAndPrimeGroupAttributes();

  result->pushdownCoveredExpr(
               result->getGroupAttr()->getCharacteristicOutputs(),
               result->getGroupAttr()->getCharacteristicInputs(),
               result->selectionPred());

  return result;

} // PhysCompoundStmtRule::nextSubstitute()

//  -----------------------------------------------------------------------
// methods for class PhysicalMapValueIdsRule
// -----------------------------------------------------------------------

PhysicalMapValueIdsRule::~PhysicalMapValueIdsRule() {} 

// This rule is context sensitive
NABoolean PhysicalMapValueIdsRule::isContextSensitive() const
{
  return TRUE;
}

NABoolean PhysicalMapValueIdsRule::topMatch (RelExpr *relExpr,
                                             Context * context)
{
  // Transform the MapValueIds to a PhysicalMapValueIds iff
  // all the selection() predicates are pushed down.
  if (NOT Rule::topMatch(relExpr, NULL))
    return FALSE;

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  if (relExpr->rppRequiresEnforcer(context->getReqdPhysicalProperty()))
    return FALSE;

  if (relExpr->selectionPred().isEmpty())
    return TRUE;
  else
    return FALSE;
}

RelExpr * PhysicalMapValueIdsRule::nextSubstitute(RelExpr * before,
                                                  Context * ,
                                                  RuleSubstituteMemory *& )
{
  CMPASSERT(before->getOperatorType() == REL_MAP_VALUEIDS);

  // Simply copy the contents of the MapValueIds from the before pattern.
  PhysicalMapValueIds * result = new(CmpCommon::statementHeap())
    PhysicalMapValueIds(*((MapValueIds *) before));

  // now set the group attributes of the result's top node
  result->setGroupAttr(before->getGroupAttr());

  return result;
}

NABoolean PhysicalMapValueIdsRule::canMatchPattern (const RelExpr *) const
{
  // map value ids are independent of required patterns, always apply this rule
  return TRUE;
}

// -----------------------------------------------------------------------
// methods for class PhysicalRelRootRule
// -----------------------------------------------------------------------

PhysicalRelRootRule::~PhysicalRelRootRule() {} 

RelExpr * PhysicalRelRootRule::nextSubstitute(RelExpr * before,
                                              Context * /*context*/,
                                              RuleSubstituteMemory *& /*mem*/)
{
  CMPASSERT(before->getOperatorType() == REL_ROOT);
  RelRoot * bef = (RelRoot *) before;

  // Simply copy the contents of the Materialize from the before pattern.
  PhysicalRelRoot * result = new(CmpCommon::statementHeap())
    PhysicalRelRoot(*bef);

  // now set the group attributes of the result's top node
  result->setGroupAttr(before->getGroupAttr());

  (void) bef->copyTopNode(result, CmpCommon::statementHeap());

  return result;
}

// -----------------------------------------------------------------------
// methods for class PhysicalTupleRule
// -----------------------------------------------------------------------

PhysicalTupleRule::~PhysicalTupleRule() {} 

NABoolean PhysicalTupleRule::topMatch (RelExpr *relExpr,
                                       Context *context)
{
  if (NOT Rule::topMatch(relExpr, context))
    return FALSE;

  // ---------------------------------------------------------------
  // allow a tuple operator to be pushed down to dp2 if flag is on
  // and do colocation check later.
  // ---------------------------------------------------------------
 if (! CURRSTMT_OPTDEFAULTS->pushDownDP2Requested() &&
     context->getReqdPhysicalProperty()->executeInDP2())
   return FALSE;

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  if (relExpr->rppRequiresEnforcer(context->getReqdPhysicalProperty()))
    return FALSE;

  return TRUE;
}

RelExpr * PhysicalTupleRule::nextSubstitute(RelExpr * before,
                                            Context * /*context*/,
                                            RuleSubstituteMemory *& /*memory*/)
{
  CMPASSERT(before->getOperatorType() == REL_TUPLE);
  Tuple * bef = (Tuple *) before;

  // Simply copy the contents of the Tuple from the before pattern.
  PhysicalTuple * result = new(CmpCommon::statementHeap())
    PhysicalTuple(*((Tuple *) before));

  // now set the group attributes of the result's top node
  result->setGroupAttr(before->getGroupAttr());

  (void) bef->copyTopNode(result, CmpCommon::statementHeap());

  result->setBlockStmt(before->isinBlockStmt());

  return result;
}

// -----------------------------------------------------------------------
// methods for class PhysicalTupleListRule
// -----------------------------------------------------------------------
PhysicalTupleListRule::~PhysicalTupleListRule() {} 

NABoolean PhysicalTupleListRule::topMatch (RelExpr *relExpr,
                                           Context *context)
{
  if (NOT Rule::topMatch(relExpr, context))
    return FALSE;

  // ---------------------------------------------------------------
  // allow a tuple operator to be pushed down to dp2 if flag is on
  // and do colocation check later.
  // ---------------------------------------------------------------
 if (! CURRSTMT_OPTDEFAULTS->pushDownDP2Requested() &&
     context->getReqdPhysicalProperty()->executeInDP2())
   return FALSE;

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  if (relExpr->rppRequiresEnforcer(context->getReqdPhysicalProperty()))
    return FALSE;

  return TRUE;
}

RelExpr * PhysicalTupleListRule::nextSubstitute(RelExpr * before,
                                            Context * /*context*/,
                                            RuleSubstituteMemory *& /*memory*/)
{
  CMPASSERT(before->getOperatorType() == REL_TUPLE_LIST);

  // Simply copy the contents of the Tuple from the before pattern.
  // no need to call copyTopNode as the copy constructor is called

  PhysicalTupleList * result = new(CmpCommon::statementHeap())
    PhysicalTupleList( *((TupleList *) before) );

  // now set the group attributes of the result's top node
  result->setGroupAttr(before->getGroupAttr());

  return result;
} //  PhysicalTupleListRule::nextSubstitute()

// -----------------------------------------------------------------------
// methods for class PhysicalExplainRule
// -----------------------------------------------------------------------

PhysicalExplainRule::~PhysicalExplainRule() {} 

RelExpr * PhysicalExplainRule::nextSubstitute(RelExpr * before,
                                              Context * /*context*/,
                                              RuleSubstituteMemory *& /*mem*/)
{
  CMPASSERT(before->getOperatorType() == REL_EXPLAIN);
  ExplainFunc * bef = (ExplainFunc *) before;

  // Simply copy the contents of the Explain from the before pattern.
  PhysicalExplain *result = new(CmpCommon::statementHeap()) PhysicalExplain();

  bef->copyTopNode(result, CmpCommon::statementHeap());

  // now set the group attributes of the result's top node
  result->selectionPred() = bef->selectionPred();
  result->setGroupAttr(before->getGroupAttr());

  return result;
}

// -----------------------------------------------------------------------
// methods for class PhysicalHiveMDRule
// -----------------------------------------------------------------------

PhysicalHiveMDRule::~PhysicalHiveMDRule() {} 

RelExpr * PhysicalHiveMDRule::nextSubstitute(RelExpr * before,
                                              Context * /*context*/,
                                              RuleSubstituteMemory *& /*mem*/)
{
  CMPASSERT(before->getOperatorType() == REL_HIVEMD_ACCESS);
  HiveMDaccessFunc * bef = (HiveMDaccessFunc *) before;

  // Simply copy the contents of the HiveMD from the before pattern.
  PhysicalHiveMD *result = new(CmpCommon::statementHeap()) PhysicalHiveMD();

  bef->copyTopNode(result, CmpCommon::statementHeap());

  // now set the group attributes of the result's top node
  result->selectionPred() = bef->selectionPred();
  result->setGroupAttr(before->getGroupAttr());

  return result;
}

// -----------------------------------------------------------------------
// methods for class PhysicalPackRule
// -----------------------------------------------------------------------

// Destructor.
PhysicalPackRule::~PhysicalPackRule()
{
}

// PhysicalPackRule::topMatch()
NABoolean PhysicalPackRule::topMatch(RelExpr* relExpr, Context* context)
{
  // Match the node type first.
  if (NOT Rule::topMatch(relExpr,context)) return FALSE;
  CMPASSERT(relExpr->getOperatorType() == REL_PACK);

  Pack* packNode = (Pack *) relExpr;

  const ReqdPhysicalProperty* rppForMe = context->getReqdPhysicalProperty();

  // If the defaults table does not indicate that this query is to be
  // executed in DP2, return FALSE
  if (rppForMe->executeInDP2() AND
      NOT CURRSTMT_OPTDEFAULTS->pushDownDP2Requested())
    return FALSE;

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  if (relExpr->rppRequiresEnforcer(rppForMe))
    return FALSE;

  if ( CURRSTMT_OPTDEFAULTS->pushDownDP2Requested() )
  {
    // If the OPTS_PUSH_DOWN_DAM CQD is set to '1', and if this rowsets total input/output
    // length * packingFactorLong_ is larger than 30000 bytes, we reject the request.

    // It is found that DP2 goes into infinite loop if we select into a rowset of
    // approximately of size > 30000. Internally the executor has a max buffer size of 31000
    // for nodes with remotepartition and 56000 for local partitions.30000 bytes limit is experimentally
    // found to be safe after taking into consideration the additional bytes that will be added for header
    // information and control information in executor.
    //
    if ((CmpCommon::getDefault(GEN_DBLIMITS_LARGER_BUFSIZE) == DF_OFF))
      {
	if ( (   ( ( relExpr->getGroupAttr()->getCharacteristicInputs().getRowLength() +
		     relExpr->getGroupAttr()->getCharacteristicOutputs().getRowLength()
		     ) * packNode->packingFactorLong()
		   )
		 > ROWSIZE_TO_EXECUTE_IN_DP2
		 )
	     AND
	     rppForMe->executeInDP2()
	     )
	  {
	    return FALSE;
	  }
      }
    else
      {
	if ( (   ( ( relExpr->getGroupAttr()->getCharacteristicInputs().getRowLength() +
		     relExpr->getGroupAttr()->getCharacteristicOutputs().getRowLength()
		     ) * packNode->packingFactorLong()
		   )
		 > ROWSIZE_TO_EXECUTE_IN_DP2_DBL
		 )
	     AND
	     rppForMe->executeInDP2()
	     )
	  {
	    return FALSE;
	  }
      }
    // For 10-050816-0628 soln. The dp2 loop happens if the data selected is more than 30,000.
    // The check above takes care of the rowset size declared in the program
    // into consideration. But, if there is mismatch between the rowsets size declared
    // and the column size that is being filled in rowset, the eid buffer will be unable
    // to handle it and will lead to dp2 loop.
    // Ex: Suppose we declare ROWSET [100] char[10] and table column col1  of size char(1000) is
    // used in a statement like "select col1 into :rowset".
    // Here we are trying to fill 100 * 1000 = 100000 bytes into a rowset of size
    // 100 * 10 = 1000 bytes and into an eid buffer of 31000.
    // This is because when you have "ROWSET [100] char[10]" and the corresponding column is
    // char[1000] the conversion happens after the pack node. Hence we are considering the record
    // size of 1000 in our calculation as this is the data that needs to be transferred. 
    // This will lead to dp2 loop since 100000 > 30000 limit which eid can work with. This check
    // will take care of this scenario.
    // Here we get the characteristic output values of pack nodes children and check if the size
    // of output is greater than 30000.
    if ((CmpCommon::getDefault(GEN_DBLIMITS_LARGER_BUFSIZE) == DF_OFF))
      { 
	RowSize
	  outputrowSize = relExpr->child(0).getGroupAttr()->getRecordLength()
	  * packNode->packingFactorLong();

	if (outputrowSize > ROWSIZE_TO_EXECUTE_IN_DP2
	    AND   rppForMe->executeInDP2())
	  {
	    return FALSE;
	  }
      }
    else
      {
	RowSize
	  outputrowSize = relExpr->child(0).getGroupAttr()->getRecordLength()
	  * packNode->packingFactorLong();

	if (outputrowSize > ROWSIZE_TO_EXECUTE_IN_DP2_DBL
	    AND   rppForMe->executeInDP2())
	  {
	    return FALSE;
	  }
      }

  }
  // ---------------------------------------------------------------------
  // Some limitations for now.
  //
  // 1. Pack node cannot satisfy any ordering requirements.
  // 2. Pack node must execute in one stream.
  // ---------------------------------------------------------------------

  if (rppForMe->getSortKey()) return FALSE;
  if (rppForMe->getArrangedCols()) return FALSE;

  if ((rppForMe->getPartitioningRequirement() != NULL) AND
      (NOT(rppForMe->getPartitioningRequirement()->isRequirementExactlyOne())))
      return FALSE;

  return TRUE;
}

RelExpr* PhysicalPackRule::nextSubstitute(RelExpr* before,
                                          Context* /*context*/,
                                          RuleSubstituteMemory*& memory)
{
  CMPASSERT(before->getOperatorType() == REL_PACK);
  CMPASSERT(memory == NULL);

  Pack* original = (Pack *) before;

  // ---------------------------------------------------------------------
  // Create empty PhyPack substitute and copy packing factor over.
  // ---------------------------------------------------------------------
  PhyPack* substitute = new (CmpCommon::statementHeap()) PhyPack();
  original->copyTopNode(substitute);

  // ---------------------------------------------------------------------
  // Fields which are not copied need to be filled in.
  // ---------------------------------------------------------------------
  substitute->setGroupAttr(original->getGroupAttr());
  substitute->child(0) = original->child(0);

  return substitute;
}

NABoolean PhysicalPackRule::canMatchPattern (const RelExpr *) const
{
  // Pack is independent of required patterns, always apply this rule
  return TRUE;
}

// -----------------------------------------------------------------------
// methods for class PhysicalTransposeRule
// -----------------------------------------------------------------------

// Destructor for the PhysicalTransposeRule
//
PhysicalTransposeRule::~PhysicalTransposeRule() {} 

// PhysicalTransposeRule::topMatch() -------------------------------------
// The method is used to determine if a rule should fire.  If
// Rule::topMatch() returns TRUE, then the relExpr has matched the
// pattern of the rule. Here we do further examination of the context
// and relExpr to determine if this rule should be applied.
//
// Parameters:
//
// RelExpr *relExpr
//  IN : The relExpr node
//
NABoolean PhysicalTransposeRule::topMatch (RelExpr *relExpr,
                                           Context *context)
{
  // Check to see if this relExpr matches the Rules pattern.
  //
  if (NOT Rule::topMatch(relExpr,context))
    return FALSE;

  // If it matched, then this must be a Transpose node.
  //
  CMPASSERT(relExpr->getOperatorType() == REL_TRANSPOSE);

  // Do a down cast. This should be safe.
  //
  Transpose *transposeNode = (Transpose *)relExpr;

  // If there are any physical requirements, make sure that
  // Transpose can satisfy them.
  // Transpose can not satisfy:
  //   - a sort requirement on any of the columns generated by transpose.
  //   - an arranged requirement on any of the columns generated by
  //     transpose.
  //   - a partitioning requirement on any of the columns generated by
  //     transpose.
  //
  // If any of these requirements exist, then this method will return
  // FALSE and the rule will not fire.
  //

  const ReqdPhysicalProperty *rppForMe = context->getReqdPhysicalProperty();

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  if (relExpr->rppRequiresEnforcer(rppForMe))
    return FALSE;

  // A set of valueIds containing the sortKeys, arrangedCols,
  // and partitioningKeys.
  //
  ValueIdSet allOrderColumns;

  if (rppForMe->getSortKey())
    allOrderColumns.insertList(*rppForMe->getSortKey());

  if (rppForMe->getArrangedCols())
    allOrderColumns += *rppForMe->getArrangedCols();

  if (rppForMe->getPartitioningRequirement() &&
      rppForMe->getPartitioningKey().entries() > 0)
    allOrderColumns += rppForMe->getPartitioningKey();

  // If any of the order columns or partitioning columns are the
  // generated transpose columns this node cannot satisfy the
  // reqirement.  If they are on the childs columns (or other
  // inputs) then it is up to the child.

  // The valueIds for the generated columns.
  //
  ValueIdSet transVals;

  for (CollIndex v = 0; v < transposeNode->transUnionVectorSize(); v++)
    transVals.insertList(transposeNode->transUnionVector()[v]);

  // If there are references to the generated columns in the
  // ordering/partitioning columns, then the rule cannot fire.
  if (allOrderColumns.referencesOneValueFromTheSet(transVals))
    return FALSE;

  // If there are no physical requirements, then this rule can fire.
  //
  return TRUE;
} // PhysicalTranspose::topMatch()


// PhysicalTransposeRule::nextSubstitute() ---------------------------------
// Generate the result of the application of a rule.
// This method returns a 'substitute' from applying the rule to the binding
// 'before'.  This method is called repeatedly until it returns a NULL
// pointer in the 'memory' argument.  Since the 'memory' argument initially
// points to a NULL pointer, if nothing is done with 'memory' this method
// will be called once per rule application.  If the rule needs to be called
// multiple times, then the method needs to alter memory to point to a non
// NULL pointer.  This 'memory' area can be used to hold state between calls
// to this method.
//
// Parameters
//
//  RelExpr *before
//   IN - A Binding for the rules pattern
//
//  Context *context
//   IN - The optimization context if the rule is applied during optimization
//        or NULL.
//
//  RuleSubstituteMemory *&memory
//   IN/OUT - a pointer to a NULL pointer to a RuleSubstituteMemory object
//            on the first call per rule application, a pointer to whatever
//            the previous call left there on subsequent calls.
//
// The PhysicalTransposeRule implementation of nextSubstitute unconditionally
// creates a PhysTranspose node from a Transpose node.  It does not examine
// the 'context' and it does not use the 'memory' (it is called once per rule
// application).  By the time nextSubstitute is called, PhysicalTranspose::
// topMatch() has returned TRUE and has qualified this binding in this context
// for this rule.
//
RelExpr * PhysicalTransposeRule::nextSubstitute(RelExpr * before,
                                                Context * /*context*/,
                                                RuleSubstituteMemory *& memory)
{
  // Just to make sure things are working as expected.
  //   (If these asserts are every taken out, the compiler will
  //    likely complain about not using the 'memory' parameter)
  //
  CMPASSERT(before->getOperatorType() == REL_TRANSPOSE);
  CMPASSERT(memory == NULL);

  // We know at this point that the binding is a Transpose node.
  //
  Transpose * transBinding = (Transpose *) before;

  // Create an empty PhysTranspose node and simply copy the
  // contents of the Transpose from the before pattern.
  //
  PhysTranspose *substitute = new(CmpCommon::statementHeap())
    PhysTranspose();

  // Copy the transpose value expressions from the (before) transpose node.
  //
  substitute->setTransUnionVectorSize(transBinding->transUnionVectorSize());

  substitute->transUnionVector() =
    new (CmpCommon::statementHeap())
    ValueIdList[transBinding->transUnionVectorSize()];

  for (CollIndex i = 0; i < transBinding->transUnionVectorSize(); i++)
    substitute->transUnionVector()[i] = transBinding->transUnionVector()[i];

  // Now copy the field defined in the RelExpr and ExprNode classes.
  //
  substitute->selectionPred() = transBinding->selectionPred();
  substitute->setGroupAttr(transBinding->getGroupAttr());
  substitute->child(0) = transBinding->child(0);

  return substitute;
}

// -----------------------------------------------------------------------
// methods for class PhysicalUnPackRowsRule
// -----------------------------------------------------------------------

// Destructor for the PhysicalUnPackRowsRule
//
PhysicalUnPackRowsRule::~PhysicalUnPackRowsRule() {} 

// PhysicalUnPackRowsRule::topMatch() -------------------------------------
// The method is used to determine if a rule should fire.  If
// Rule::topMatch() returns TRUE, then the relExpr has matched the
// pattern of the rule. Here we do further examination of the context
// and relExpr to determine if this rule should be applied.
//
// Parameters:
//
// RelExpr *relExpr
//  IN : The relExpr node
//
NABoolean PhysicalUnPackRowsRule::topMatch (RelExpr *relExpr,
                                            Context *context)
{
  // Check to see if this relExpr matches the Rules pattern.
  //
  if (NOT Rule::topMatch(relExpr,context))
    return FALSE;

  // If it matched, then this must be an UnPackRows node.
  //
  CMPASSERT(relExpr->getOperatorType() == REL_UNPACKROWS);

  // Do a down cast. This should be safe.
  //
  UnPackRows *UnPackRowsNode = (UnPackRows *)relExpr;

  // Make sure UnPackRows can satisfy the physical requirements.
  // UnPackRows can not satisfy:
  //   - a sort requirement on any of the columns generated by UnPackRows.
  //   - an arranged requirement on any of the columns generated by
  //     UnPackRows.
  //   - a partitioning requirement on any of the columns generated by
  //     UnPackRows.
  //
  // If any of these requirements exist, then this method will return
  // FALSE and the rule will not fire.
  //

  const ReqdPhysicalProperty *rppForMe = context->getReqdPhysicalProperty();

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  if (relExpr->rppRequiresEnforcer(rppForMe))
    return FALSE;

  // We check whether above-DP2 unpacking is allowed only if
  // the source is a table.
  //
  if ( UnPackRowsNode->unPackedTable() )
  {

    // If the size of the input row is larger than about 31000 / 2,
    // then the UnPackRows node should be performed in DP2.  This is
    // because the File System has a buffer limit (EXPAND_LIMIT) equal
    // to 31000 (see dml/fs_session.cpp and dml/fsglobals.h) and two
    // rows must be able to fit in the buffer.
    //
    // WARNING - If the value of EXPAND_LIMIT ever changes (becomes
    // smaller), this code could break.  A more permanent solution
    // would be to allow the DP2->ESP interface to handle any size
    // message, breaking it up into smaller chunks if necessary.

    // This value should always be equal to the #define EXPAND_LIMIT
    // as defined in dml/fsglobals.h.
    //
    const Int32 expandLimit = 31000;

    RowSize inputRowSize =
      UnPackRowsNode->child(0).getGroupAttr()->getRecordLength();

    if((NOT rppForMe->executeInDP2()) AND
       (inputRowSize > (expandLimit/2)))
      return FALSE;
  }
  else { // we're a rowset unpack
    // rowset unpack should run in master because it's inefficient and
    // risky to send rowsets from master to an ESP. An ESP eventually
    // flows the rowset's data back to the master. An ESP parallel plan
    // fragment with a large rowset can exceed IPC size limit and crash
    // at run-time (see genesis case 10-060510-3471).
    if (UnPackRowsNode->child(0).getGroupAttr()->
        getResultCardinalityForEmptyInput() >
        ActiveSchemaDB()->getDefaults().getAsLong(COMP_INT_5)) {
      if (rppForMe->getPlanExecutionLocation() != EXECUTE_IN_MASTER) {
        return FALSE;
      }
    }
  }

  // A set of valueIds containing the sortKeys, arrangedCols,
  // and partitioningKeys.
  //
  ValueIdSet allOrderColumns;

  if (rppForMe->getSortKey())
    allOrderColumns.insertList(*rppForMe->getSortKey());

  if (rppForMe->getArrangedCols())
    allOrderColumns += *rppForMe->getArrangedCols();

  if (rppForMe->getPartitioningRequirement() &&
      rppForMe->getPartitioningKey().entries() > 0)
    allOrderColumns += rppForMe->getPartitioningKey();

  // If any of the order columns or partitioning columns are the
  // generated UnPackRows columns this node cannot satisfy the
  // reqirement.

  // If there are no requirements, then this rule should fire.
  //
  if (NOT allOrderColumns.isEmpty())
    return FALSE;

  if (rppForMe->executeInDP2())
  {
     // if we are a rowsetIterator, return false.
    if (UnPackRowsNode->isRowsetIterator())
        return FALSE;
  }

  return TRUE;
} // PhysicalUnPackRows::topMatch()


// PhysicalUnPackRowsRule::nextSubstitute() ---------------------------------
// Generate the result of the application of a rule.
// This method returns a 'substitute' from applying the rule to the binding
// 'before'.  This method is called repeatedly until it returns a NULL
// pointer in the 'memory' argument.  Since the 'memory' argument initially
// points to a NULL pointer, if nothing is done with 'memory' this method
// will be called once per rule application.  If the rule needs to be called
// multiple times, then the method needs to alter memory to point to a non
// NULL pointer.  This 'memory' area can be used to hold state between calls
// to this method.
//
// Parameters
//
//  RelExpr *before
//   IN - A Binding for the rules pattern
//
//  Context *context
//   IN - The optimization context if the rule is applied during optimization
//        or NULL.
//
//  RuleSubstituteMemory *&memory
//   IN/OUT - a pointer to a NULL pointer to a RuleSubstituteMemory object
//            on the first call per rule application, a pointer to whatever
//            the previous call left there on subsequent calls.
//
// The PhysicalUnPackRowsRule implementation of nextSubstitute unconditionally
// creates a PhysUnPackRows node from a UnPackRows node.  It does not examine
// the 'context' and it does not use the 'memory' (it is called once per rule
// application).  By the time nextSubstitute is called, PhysicalUnPackRowsRule
// ::topMatch() has returned TRUE and has qualified this binding in this
// context for this rule.
//
RelExpr * PhysicalUnPackRowsRule::nextSubstitute(RelExpr * before,
                                                 Context * /*context*/,
                                                 RuleSubstituteMemory *& memory)
{
  // Just to make sure things are working as expected.
  //   (If these asserts are every taken out, the compiler will
  //    likely complain about not using the 'memory' parameter)
  //
  CMPASSERT(before->getOperatorType() == REL_UNPACKROWS);
  CMPASSERT(memory == NULL);

  // We know at this point that the binding is a UnPackRows node.
  //
  UnPackRows *unPackBinding = (UnPackRows *)before;

  // Create an empty PhysUnPackRows node and simply copy the
  // contents of the UnPackRows from the before pattern.
  //
  PhysUnPackRows *substitute = new(CmpCommon::statementHeap())
    PhysUnPackRows();

  // Copy the UnPackRows value expressions from the (before) UnPackRows node.
  //
  substitute->unPackExpr() = unPackBinding->unPackExpr();

  substitute->packingFactor() = unPackBinding->packingFactor();

  substitute->indexValue() = unPackBinding->indexValue();

  substitute->unPackedTable() = unPackBinding->unPackedTable();

  substitute->setRowsetIterator(unPackBinding->isRowsetIterator());

  substitute->setTolerateNonFatalError(unPackBinding->getTolerateNonFatalError());


  substitute->setRowwiseRowset(unPackBinding->rowwiseRowset());
  substitute->setRwrsInputSizeExpr(unPackBinding->rwrsInputSizeExpr());
  substitute->setRwrsMaxInputRowlenExpr(unPackBinding->rwrsMaxInputRowlenExpr());
  substitute->setRwrsBufferAddrExpr(unPackBinding->rwrsBufferAddrExpr());
  substitute->setRwrsOutputVids(unPackBinding->rwrsOutputVids());
  
  // Now copy the field defined in the RelExpr and ExprNode classes.
  //
  substitute->selectionPred() = unPackBinding->selectionPred();
  substitute->setGroupAttr(unPackBinding->getGroupAttr());
  substitute->child(0) = unPackBinding->child(0);

  return substitute;
}

NABoolean PhysicalUnPackRowsRule::canMatchPattern (const RelExpr *) const
{
  // UnPack is independent of required patterns, always apply this rule
  return TRUE;
}

// -----------------------------------------------------------------------
// methods for class SortEnforcerRule
// -----------------------------------------------------------------------

SortEnforcerRule::~SortEnforcerRule() {} 

NABoolean SortEnforcerRule::topMatch (RelExpr *  /* relExpr */,
                                      Context *context)
{
  const ReqdPhysicalProperty *rppForMe = context->getReqdPhysicalProperty();

  // QSTUFF
  // can't sort streams
  if (context->getGroupAttr()->isStream()){
    return FALSE;
  }

  // Don't want to tangle sorts and GET_NEXT_N commands yet
  if (context->getGroupAttr()->isEmbeddedUpdateOrDelete())
    return FALSE;
  // QSTUFF  ^^

  // If no required physical properties, there won't be any sort
  // requirement to sort for, so return FALSE. Note there can only
  // be no rpp if the operator the rule is firing on is RelRoot.
  if (rppForMe == NULL)
    return FALSE;

  // make sure a sort order is required
  if (NOT  context->requiresOrder())
    return FALSE;

  //  don't run in DP2
  if (rppForMe->executeInDP2())
    return FALSE;

  PartitioningRequirement* partReqForMe =
    rppForMe->getPartitioningRequirement();

  // If a partitioning requirement exists and it requires broadcast
  // replication, then return FALSE. Only an exchange operator
  // can satisfy a broadcast replication partitioning requirement.
  if ((partReqForMe != NULL) AND
      partReqForMe->isRequirementReplicateViaBroadcast())
    return FALSE;

  SortOrderTypeEnum sortOrderTypeReq =
    rppForMe->getSortOrderTypeReq();

  // If a sort order type requirement of ESP_NO_SORT, DP2, or
  // DP2_OR_ESP_NO_SORT exists, then return FALSE now.
  // A sort operator can only produce a sort order type of
  // ESP_VIA_SORT.
  if ((sortOrderTypeReq == ESP_NO_SORT_SOT) OR
      (sortOrderTypeReq == DP2_SOT) OR
      (sortOrderTypeReq == DP2_OR_ESP_NO_SORT_SOT))
    return FALSE;

  return TRUE;
}

RelExpr * SortEnforcerRule::nextSubstitute(RelExpr * before,
                                           Context * context,
                                           RuleSubstituteMemory *&)
{
  const ReqdPhysicalProperty* const rppForMe =
    context->getReqdPhysicalProperty();
  Sort *result = new(CmpCommon::statementHeap()) Sort(before);

  // store the required orders in the sort node
  if (rppForMe->getSortKey())
  {
    result->getSortKey() = *rppForMe->getSortKey();
  }

  if (rppForMe->getArrangedCols())
  {
    result->getArrangedCols() = *rppForMe->getArrangedCols();
  }

  result->setGroupAttr(before->getGroupAttr());
  return result;
}

Int32 SortEnforcerRule::promiseForOptimization(RelExpr *,
                                               Guidance *,
                                               Context *)
{
  // A sort enforcer is less promising than an implementation or
  // a transformation rule. This way, we will optimize the expressions
  // with a required order before trying the enforcer. When we try the
  // enforcer, the expressions with required order supply good cost bounds.
  return DefaultEnforcerRulePromise;
}

// -----------------------------------------------------------------------
// methods for class ExchangeEnforcerRule
// -----------------------------------------------------------------------

ExchangeEnforcerRule::~ExchangeEnforcerRule() {} 

NABoolean ExchangeEnforcerRule::topMatch(RelExpr *  /* relExpr */,
                                         Context *context)
{
  const ReqdPhysicalProperty *rppForMe = context->getReqdPhysicalProperty();

  // If no required physical properties, there won't be any location
  // or partitioning requirement to enforce, so return FALSE.
  // Note there can only be no rpp if the operator the rule is firing
  // on is RelRoot.
  if (rppForMe == NULL)
    return FALSE;

  // the exchange enforcer itself can't run in DP2, in all other cases
  // it could enforce either location or partitioning
  if (rppForMe->executeInDP2())
    return FALSE;

  return TRUE;
}

RelExpr * ExchangeEnforcerRule::nextSubstitute(RelExpr * before,
                                               Context * context,
                                               RuleSubstituteMemory *&)
{
  Exchange *result = new(CmpCommon::statementHeap())
    Exchange(before);

  result->setGroupAttr(before->getGroupAttr());
  return result;
}

Int32 ExchangeEnforcerRule::promiseForOptimization(RelExpr *,
                                                   Guidance *,
                                                   Context *)
{
  // A sort enforcer is less promising than an implementation or
  // a transformation rule. This way, we will optimize the expressions
  // with a required order before trying the enforcer. When we try the
  // enforcer, the expressions with required order supply good cost bounds.
  return DefaultEnforcerRulePromise;
}

// -----------------------------------------------------------------------
// methods for class PhysicalSequenceRule
// -----------------------------------------------------------------------
// Destructor for the PhysicalSequenceRule
//
PhysicalSequenceRule::~PhysicalSequenceRule() {} 

// PhysicalSequenceRule::topMatch() ----------------------------------
// The method is used to determine if a rule should fire.  If
// Rule::topMatch() returns TRUE, then the relExpr has matched the
// pattern of the rule. Here we do further examination of the context
// and relExpr to determine if this rule should be applied.
//
// Parameters:
//
// RelExpr *relExpr
//  IN : The relExpr node
//
NABoolean PhysicalSequenceRule::topMatch (RelExpr *relExpr,
                                          Context *context)
{
  // Check to see if this relExpr matches the Rules pattern.
  //
  if (NOT Rule::topMatch(relExpr,context))
    return FALSE;

  // If it matched, then this must be an RelSequence node.
  //
  CMPASSERT(relExpr->getOperatorType() == REL_SEQUENCE);

  // QSTUFF
  // Sequence functions don't work with embedded update/delete
  CMPASSERT(NOT(relExpr->getGroupAttr()->isEmbeddedUpdateOrDelete()));
  // QSTUFF

  const RelSequence *sequenceNode = (RelSequence *)relExpr;

  const ReqdPhysicalProperty *rppForMe = context->getReqdPhysicalProperty();

  // For now, the sequence node can not execute in DP2, since it
  // allocates a bunch of memory.
  //
  if(rppForMe->executeInDP2()) {
    return FALSE;
  }

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  if (relExpr->rppRequiresEnforcer(rppForMe))
    return FALSE;

  ValueIdList reqdOrder =
    sequenceNode->mapSortKey(sequenceNode->partition());

  reqdOrder.insert(sequenceNode->mapSortKey(sequenceNode->requiredOrder()));

  // The sequence node will produce its results in the order of the
  // required order.
  //
  if(rppForMe->getSortKey() AND
     (reqdOrder.satisfiesReqdOrder(
       *rppForMe->getSortKey()) == DIFFERENT_ORDER))
  {
    return FALSE;
  }

  if(rppForMe->getArrangedCols() AND
     (reqdOrder.satisfiesReqdArrangement(
         *rppForMe->getArrangedCols()) == DIFFERENT_ORDER))
  {
    return FALSE;
  }

  return TRUE;
} // PhysicalSequenceRule::topMatch()


// PhysicalSequenceRule::nextSubstitute() ----------------------------
// Generate the result of the application of a rule.  This method
// returns a 'substitute' from applying the rule to the binding
// 'before'.  This method is called repeatedly until it returns a NULL
// pointer in the 'memory' argument.  Since the 'memory' argument
// initially points to a NULL pointer, if nothing is done with
// 'memory' this method will be called once per rule application.  If
// the rule needs to be called multiple times, then the method needs
// to alter memory to point to a non NULL pointer.  This 'memory' area
// can be used to hold state between calls to this method.
//
// Parameters
//
//  RelExpr *before
//   IN - A Binding for the rules pattern
//
//  Context *context
//   IN - The optimization context if the rule is applied during optimization
//        or NULL.
//
//  RuleSubstituteMemory *&memory
//   IN/OUT - a pointer to a NULL pointer to a RuleSubstituteMemory object
//            on the first call per rule application, a pointer to whatever
//            the previous call left there on subsequent calls.
//
// The PhysicalSequenceRule implementation of nextSubstitute
// unconditionally creates a PhysSequence node from a RelSequence
// node.  It does not examine the 'context' and it does not use the
// 'memory' (it is called once per rule application).  By the time
// nextSubstitute is called, PhysicalSequenceRule::topMatch() has
// returned TRUE and has qualified this binding in this context for
// this rule.
//
RelExpr * PhysicalSequenceRule::nextSubstitute(RelExpr * before,
                                               Context * /*context*/,
                                               RuleSubstituteMemory *& memory)
{
  // Just to make sure things are working as expected.
  //   (If these asserts are every taken out, the compiler will
  //    likely complain about not using the 'memory' parameter)
  //
  CMPASSERT(before->getOperatorType() == REL_SEQUENCE);
  CMPASSERT(memory == NULL);

  // We know at this point that the binding is a RelSequence node.
  //
  const RelSequence *relSequenceBinding = (RelSequence *)before;

  // Create an empty PhysSequence node and simply copy the
  // contents of the RelSequence from the before pattern.
  //
  PhysSequence *substitute = new(CmpCommon::statementHeap())
    PhysSequence();

  // Copy the expressions from the (before) RelSequence node.
  //
  substitute->setRequiredOrder(relSequenceBinding->requiredOrder());

  substitute->setPartition(relSequenceBinding->partition());

  substitute->setSequenceFunctions(relSequenceBinding->sequenceFunctions());

  substitute->setSequencedColumns(relSequenceBinding->sequencedColumns());

  substitute->setCancelExpr(relSequenceBinding->cancelExpr());

  substitute->setHasTDFunctions(relSequenceBinding->getHasTDFunctions());

  substitute->setHasOlapFunctions(relSequenceBinding->getHasOlapFunctions());

  // Now copy the field defined in the RelExpr and ExprNode classes.
  //
  substitute->selectionPred() = relSequenceBinding->getSelectionPred();
  substitute->setGroupAttr(relSequenceBinding->getGroupAttr());
  substitute->child(0) = relSequenceBinding->child(0);

  substitute->retrieveCachedHistoryInfo((RelSequence *) relSequenceBinding);

  return substitute;
}


// -----------------------------------------------------------------------
// methods for class PhysicalSampleRule
// -----------------------------------------------------------------------

// Destructor for the PhysicalSampleRule
//
PhysicalSampleRule::~PhysicalSampleRule() {} 

// PhysicalSampleRule::topMatch() -------------------------------------
// The method is used to determine if a rule should fire.  If
// Rule::topMatch() returns TRUE, then the relExpr has matched the
// pattern of the rule. Here we do further examination of the context
// and relExpr to determine if this rule should be applied.
//
// Parameters:
//
// RelExpr *relExpr
//  IN : The relExpr node
//
NABoolean PhysicalSampleRule::topMatch (RelExpr *relExpr,
                                        Context *context)
{
  // Check to see if this relExpr matches the Rules pattern.
  //
  if (NOT Rule::topMatch(relExpr,context))
    return FALSE;

  // If it matched, then this must be a RelSample node.
  //
  CMPASSERT(relExpr->getOperatorType() == REL_SAMPLE);

  RelSample *sampleNode = (RelSample *)relExpr;

  // If this is a CLUSTER sample, then this may have already been
  // transformed into a sample scan by the SampleScanRule
  // (TransRule.cpp) and we do not want to implement this with a
  // physical sample node (this rule).  If the SampleScanRule
  // succeeded, then do not apply this rule.  On the other hand, if
  // for whatever reason, the SampleScanRule did not succeed, then use
  // this rule to implement a physical scan node.  In this case an
  // appropriate error message will be returned indicating that a
  // plan with CLUSTER sampling could not be found.  Note that the
  // promiseForOptimization for the SampleScanRule has been raised to
  // force the SampleScanRule::nextSubstitute (but not necessarily the
  // topMatch method) to be applied before this Rule.
  //
  if (sampleNode->sampleType() == RelSample::CLUSTER &&
      sampleNode->sampleScanSucceeded())
  {
    return FALSE;
  }

  const ReqdPhysicalProperty *rppForMe = context->getReqdPhysicalProperty();

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  if (relExpr->rppRequiresEnforcer(rppForMe))
    return FALSE;

  ValueIdList reqdOrder =
    sampleNode->mapSortKey(sampleNode->requiredOrder());

  // The Sample node will produce its results in the order of the
  // required order.
  //
  if(rppForMe->getSortKey() AND
     (reqdOrder.satisfiesReqdOrder(
       *rppForMe->getSortKey()) == DIFFERENT_ORDER))
  {
    return FALSE;
  }

  if(rppForMe->getArrangedCols() AND
     (reqdOrder.satisfiesReqdArrangement(
        *rppForMe->getArrangedCols()) == DIFFERENT_ORDER))
  {
    return FALSE;
  }

  return TRUE;
} // PhysicalSample::topMatch()


// PhysicalSampleRule::nextSubstitute() ---------------------------------
// Generate the result of the application of a rule.
// This method returns a 'substitute' from applying the rule to the binding
// 'before'.  This method is called repeatedly until it returns a NULL
// pointer in the 'memory' argument.  Since the 'memory' argument initially
// points to a NULL pointer, if nothing is done with 'memory' this method
// will be called once per rule application.  If the rule needs to be called
// multiple times, then the method needs to alter memory to point to a non
// NULL pointer.  This 'memory' area can be used to hold state between calls
// to this method.
//
// Parameters
//
//  RelExpr *before
//   IN - A Binding for the rules pattern
//
//  Context *context
//   IN - The optimization context if the rule is applied during optimization
//        or NULL.
//
//  RuleSubstituteMemory *&memory
//   IN/OUT - a pointer to a NULL pointer to a RuleSubstituteMemory object
//            on the first call per rule application, a pointer to whatever
//            the previous call left there on subsequent calls.
//
// The PhysicalSampleRule implementation of nextSubstitute unconditionally
// creates a PhysSample node from a RelSample node.  It does not examine
// the 'context' and it does not use the 'memory' (it is called once per rule
// application).  By the time nextSubstitute is called, PhysicalSampleRule
// ::topMatch() has returned TRUE and has qualified this binding in this
// context for this rule.
//
RelExpr * PhysicalSampleRule::nextSubstitute(RelExpr * before,
                                             Context * /*context*/,
                                             RuleSubstituteMemory *& memory)
{
  // Just to make sure things are working as expected.
  //   (If these asserts are every taken out, the compiler will
  //    likely complain about not using the 'memory' parameter)
  //
  CMPASSERT(before->getOperatorType() == REL_SAMPLE);
  CMPASSERT(memory == NULL);

  // We know at this point that the binding is a UnPackRows node.
  //
  RelSample *sampleBinding = (RelSample *)before;

  // If this is a CLUSTER sample, then this should have been
  // transformed into a sample scan by the SampleScanRule
  // (TransRule.cpp) and we do not want to implement this with a
  // physical sample node (this rule).  If the SampleScanRule
  // succeeded, then do not apply this rule.  On the other hand, if
  // for whatever reason, the SampleScanRule did not succeed, then use
  // this rule to implement a physical scan node.  In this case an
  // appropriate error message will be returned indicating that a
  // plan with CLUSTER sampling could not be found.  Note that the
  // promiseForOptimization for the SampleScanRule has been raised to
  // force the SampleScanRule::nextSubstitute (but not necessarily the
  // topMatch method) to be applied before this Rule's nextSubstitute.
  //
  if (sampleBinding->sampleType() == RelSample::CLUSTER &&
      sampleBinding->sampleScanSucceeded())
  {
    return NULL;
  }

  // Create an empty PhysSample node and simply copy the
  // contents of the RelSample from the before pattern.
  //
  PhysSample *substitute = new(CmpCommon::statementHeap())
    PhysSample();

  // Copy the RelSample value expressions from the (before) UnPackRows node.
  //
  substitute->sampleType(sampleBinding->sampleType());

  substitute->balanceExpr() = sampleBinding->balanceExpr();

  substitute->sampledColumns() = sampleBinding->sampledColumns();

  substitute->requiredOrder() = sampleBinding->requiredOrder();

  // Now copy the field defined in the RelExpr and ExprNode classes.
  //
  substitute->selectionPred() = sampleBinding->selectionPred();
  substitute->setGroupAttr(sampleBinding->getGroupAttr());
  substitute->child(0) = sampleBinding->child(0);

  return substitute;
}

// -----------------------------------------------------------------------
// methods for class PhysicalSPProxyFuncRule
// -----------------------------------------------------------------------
PhysicalSPProxyFuncRule::~PhysicalSPProxyFuncRule()
{}

NABoolean PhysicalSPProxyFuncRule::topMatch(RelExpr *relExpr,
                                            Context *context)
{
  if (NOT Rule::topMatch(relExpr,context))
    return FALSE;
  
  return TRUE;
}

RelExpr *PhysicalSPProxyFuncRule::nextSubstitute(RelExpr *before,
                                                 Context *context,
                                                 RuleSubstituteMemory *&)
{
  // Create a new physical node and copy all attributes. The parent
  // class method RelExpr::copyTopNode() does a "deeper" copy than the
  // RelExpr copy constructor.
  PhysicalSPProxyFunc *result = new (CmpCommon::statementHeap()) 
                                                     PhysicalSPProxyFunc();

  before->copyTopNode(result, CmpCommon::statementHeap());

  // Some fields are not copied and need to be filled in.
  result->setGroupAttr(before->getGroupAttr());
  
  return result;
}

// -----------------------------------------------------------------------
// methods for class PhysicalExtractSourceRule
// -----------------------------------------------------------------------
PhysicalExtractSourceRule::~PhysicalExtractSourceRule() {} 

NABoolean PhysicalExtractSourceRule::topMatch(RelExpr *relExpr,
                                              Context *context)
{
  if (NOT Rule::topMatch(relExpr,context))
    return FALSE;
  return TRUE;
}

RelExpr *PhysicalExtractSourceRule::nextSubstitute(RelExpr *before,
                                                   Context *context,
                                                   RuleSubstituteMemory *&)
{
  // Create a new physical node and copy all attributes. The parent
  // class method RelExpr::copyTopNode() does a "deeper" copy than the
  // RelExpr copy constructor.
  PhysicalExtractSource *result = new (CmpCommon::statementHeap()) 
                                                    PhysicalExtractSource();
  before->copyTopNode(result, CmpCommon::statementHeap());
  
  // Some fields are not copied and need to be filled in.
  result->setGroupAttr(before->getGroupAttr());
  
  return (RelExpr *) result;
}

PhysicalIsolatedScalarUDFRule::~PhysicalIsolatedScalarUDFRule() {} 

NABoolean PhysicalIsolatedScalarUDFRule::topMatch (RelExpr *relExpr,
                                                Context *context)
{
  if (NOT Rule::topMatch(relExpr, context))
    return FALSE;

  const ReqdPhysicalProperty* const rppForMe =
    context->getReqdPhysicalProperty();

  // UDFs cannot execute in DP2 so if we are asked to do so, refuse.
  if (rppForMe->executeInDP2())
    return FALSE;

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  if (relExpr->rppRequiresEnforcer(rppForMe))
    return FALSE;

  if ((rppForMe->getPartitioningRequirement() != NULL) AND
      (NOT(rppForMe->getPartitioningRequirement()->isRequirementExactlyOne()))
      AND
      (NOT(rppForMe->getPartitioningRequirement()->isRequirementReplicateNoBroadcast())))
    return FALSE;


  return TRUE;
}

RelExpr * PhysicalIsolatedScalarUDFRule::nextSubstitute(RelExpr * before,
                                            Context * /*context*/,
                                            RuleSubstituteMemory *& /*memory*/)
{
  CMPASSERT(before->getOperatorType() == REL_ISOLATED_SCALAR_UDF);
  IsolatedScalarUDF * bef = (IsolatedScalarUDF *) before;

  // Simply copy the contents of the IsolatedScalarUDF from the before pattern.
  PhysicalIsolatedScalarUDF *result = new (CmpCommon::statementHeap())
    PhysicalIsolatedScalarUDF(CmpCommon::statementHeap());

  bef->copyTopNode(result, CmpCommon::statementHeap());

  // now set the group attributes of the result's top node
  result->setGroupAttr(before->getGroupAttr());

  return result;
}

PhysicalTMUDFRule::~PhysicalTMUDFRule() {} 

NABoolean PhysicalTMUDFRule::topMatch (RelExpr *relExpr,
                                       Context *context)
{
  if (NOT relExpr->getOperator().match(REL_ANY_TABLE_MAPPING_UDF))
    return FALSE;
  if (relExpr->isPhysical())
    return FALSE;

  if (context->getReqdPhysicalProperty()->executeInDP2())
    return FALSE;

  const ReqdPhysicalProperty* const rppForMe =
    context->getReqdPhysicalProperty();
  
  // Check for required physical properties that require an enforcer
  // operator to succeed.
  if (relExpr->rppRequiresEnforcer(rppForMe))
    return FALSE;

  return TRUE;
}

RelExpr * PhysicalTMUDFRule::nextSubstitute(RelExpr * before,
                                            Context * /*context*/,
                                            RuleSubstituteMemory *& /*memory*/)
{
  CMPASSERT(before->getOperator().match(REL_ANY_TABLE_MAPPING_UDF));
  TableMappingUDF * bef = (TableMappingUDF *) before;

  // Simply copy the contents of the TableMappingUDF from the before pattern.
  PhysicalTableMappingUDF *result = new (CmpCommon::statementHeap())
    PhysicalTableMappingUDF(before->getArity(), CmpCommon::statementHeap());
  bef->TableMappingUDF::copyTopNode(result, CmpCommon::statementHeap());

  // now set the group attributes of the result's top node
  result->setGroupAttr(before->getGroupAttr());
  for (Int32 i=0; i < before->getArity(); i++)
    result->child(i) = before->child(i);

  return result;
}

NABoolean PhysicalTMUDFRule::canMatchPattern (const RelExpr *pattern) const
{
  switch (pattern->getOperatorType())
    {
    case REL_ANY_TABLE_MAPPING_UDF:
    case REL_ANY_LEAF_TABLE_MAPPING_UDF:
    case REL_ANY_UNARY_TABLE_MAPPING_UDF:
    case REL_ANY_BINARY_TABLE_MAPPING_UDF:
      return TRUE;

    default:
      if (pattern->getOperator().isWildcard())
        return FALSE;
      else
        return pattern->getOperator().match(REL_ANY_TABLE_MAPPING_UDF);
    }
}

PhysicalFastExtractRule::~PhysicalFastExtractRule() {}

NABoolean PhysicalFastExtractRule::topMatch (RelExpr *relExpr,
                                                Context *context)
{
  if (NOT relExpr->getOperator().match(REL_FAST_EXTRACT))
    return FALSE;
  if (relExpr->isPhysical())
    return FALSE;

    if ((context->getReqdPhysicalProperty()->executeInDP2()))
      return FALSE;

  const ReqdPhysicalProperty* const rppForMe =
    context->getReqdPhysicalProperty();

  // Check for required physical properties that require an enforcer
  // operator to succeed.
  if (relExpr->rppRequiresEnforcer(rppForMe))
    return FALSE;

  if ((rppForMe->getPartitioningRequirement() != NULL) AND
      (rppForMe->getPartitioningRequirement()->isRequirementReplicateNoBroadcast()))
    return FALSE;

  return TRUE;
}

RelExpr * PhysicalFastExtractRule::nextSubstitute(RelExpr * before,
                                            Context * /*context*/,
                                            RuleSubstituteMemory *& /*memory*/)
{
  CMPASSERT(before->getOperatorType() == REL_FAST_EXTRACT);
  FastExtract * bef = (FastExtract *) before;

  // Simply copy the contents of the TableMappingUDF from the before pattern.
  PhysicalFastExtract *result = new (CmpCommon::statementHeap())
    PhysicalFastExtract(before->child(0)->castToRelExpr(),
    					CmpCommon::statementHeap());
  bef->copyTopNode(result, CmpCommon::statementHeap());

  // now set the group attributes of the result's top node
  result->setGroupAttr(before->getGroupAttr());

  return result;
}
