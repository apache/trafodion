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
* File:         TransRule.C
* Description:  DBI-defined transformation rules
*
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
#include "AppliedStatMan.h"
#include "opt.h"
#include "PhyProp.h"
#include "TransRule.h"
#include "AllRelExpr.h"
#include "RelSample.h"
#include "AllItemExpr.h"
#include "EstLogProp.h"
#include "CmpContext.h"
#include "NormWA.h"
#include "Analyzer.h"
#include "MultiJoin.h"

// -----------------------------------------------------------------------
// Global variables
// -----------------------------------------------------------------------
THREAD_P NAUnsigned              RoutineJoinToTSJRuleNumber;
THREAD_P NAUnsigned              JoinToTSJRuleNumber;
THREAD_P NAUnsigned              JoinCommutativityRuleNumber;
THREAD_P NAUnsigned              JoinLeftShiftRuleNumber;
THREAD_P NAUnsigned              FilterRule0Number;
THREAD_P NAUnsigned              FilterRule1Number;
THREAD_P NAUnsigned              FilterRule2Number;
THREAD_P NAUnsigned              MJEnumRuleNumber;
THREAD_P NAUnsigned              MJStarJoinIRuleNumber;
THREAD_P NAUnsigned              MJStarJoinIIRuleNumber;
THREAD_P NAUnsigned              MJExpandRuleNumber;
THREAD_P NAUnsigned              MVQRRewriteRuleNumber;

// -----------------------------------------------------------------------
// Function to add transformation rules to the rule set
// -----------------------------------------------------------------------
void CreateTransformationRules(RuleSet* set)
{
  Rule *r;

  r = new (CmpCommon::contextHeap())

            MJStarJoinIRule
               ("MultiJoin StarJoin I Rule",
                NULL,
                NULL
                );
      set->insert(r);
      set->enable (r->getNumber(),
                             set->getFirstPassNumber());

  MJStarJoinIRuleNumber = r->getNumber();

  r = new (CmpCommon::contextHeap())

            MJStarJoinIIRule
               ("MultiJoin StarJoin II Rule",
                NULL,
                NULL
                );
      set->insert(r);
      set->enable (r->getNumber(),
                             set->getFirstPassNumber());

  MJStarJoinIIRuleNumber = r->getNumber();

  r = new (CmpCommon::contextHeap())
          MVQRRule
             ("MVQR Rewrite Rule",
              NULL,
              NULL
              );
  set->insert(r);
  set->enable (r->getNumber(),
                         set->getSecondPassNumber());

  MVQRRewriteRuleNumber = r->getNumber();

  r = new (CmpCommon::contextHeap())
          MVQRScanRule
             ("MVQR Scan Rewrite Rule",
              NULL,
              NULL
              );
  set->insert(r);
  set->enable (r->getNumber(),
                         set->getSecondPassNumber());

  r = new (CmpCommon::contextHeap())
          MJExpandRule
             ("MultiJoin Expand Rule",
              NULL,
              NULL
              );
  set->insert(r);
  set->enable (r->getNumber(),
                         set->getSecondPassNumber());

  MJExpandRuleNumber = r->getNumber();

  r = new (CmpCommon::contextHeap())
          MJEnumRule
             ("MultiJoin Enumeration Rule",
              NULL,
              NULL
              );
  set->insert(r);
  set->enable (r->getNumber(),
                         set->getSecondPassNumber());

  MJEnumRuleNumber = r->getNumber();

  r = new(CmpCommon::contextHeap())
        JoinCommutativityRule ("Join commutativity",
                               new (CmpCommon::contextHeap())
                                 WildCardOp(REL_ANY_NON_TS_INNER_JOIN,
                                      0,
                                      new (CmpCommon::contextHeap())
                                        CutOp(0, CmpCommon::contextHeap()),
                                      new (CmpCommon::contextHeap())
                                        CutOp(1, CmpCommon::contextHeap()),
                                      CmpCommon::contextHeap()),
                               new (CmpCommon::contextHeap())
                                 WildCardOp(REL_ANY_NON_TS_INNER_JOIN,
                                      0,
                                      new (CmpCommon::contextHeap())
                                        CutOp(1, CmpCommon::contextHeap()),
                                      new (CmpCommon::contextHeap())
                                        CutOp(0, CmpCommon::contextHeap()),
                                      CmpCommon::contextHeap()));
  set->insert(r);

  set->enable(r->getNumber(),
                        set->getSecondPassNumber());

  JoinCommutativityRuleNumber = r->getNumber();

  r = new (CmpCommon::contextHeap())
          JoinLeftShiftRule
             ("Left shift rule for inner joins",
              new (CmpCommon::contextHeap())
                   WildCardOp(REL_ANY_NON_TSJ_JOIN,
                              0,
                              new (CmpCommon::contextHeap())
                                  WildCardOp(REL_ANY_NON_TSJ_JOIN,
                                             1,
                                             new (CmpCommon::contextHeap())
                                               CutOp(0,
                                                     CmpCommon::contextHeap()),
                                             new (CmpCommon::contextHeap())
                                               CutOp(1, CmpCommon::contextHeap()),
                                             CmpCommon::contextHeap()),
                              new (CmpCommon::contextHeap())
                                CutOp(2, CmpCommon::contextHeap()),
                              CmpCommon::contextHeap()),
              new (CmpCommon::contextHeap())
                  WildCardOp(REL_ANY_NON_TSJ_JOIN,
                             1,
                             new (CmpCommon::contextHeap())
                               WildCardOp(REL_ANY_NON_TSJ_JOIN,
                                          0,
                                          new(CmpCommon::contextHeap())
                                            CutOp(0, CmpCommon::contextHeap()),
                                          new(CmpCommon::contextHeap())
                                            CutOp(2, CmpCommon::contextHeap()),
                                          CmpCommon::contextHeap()),
                             new (CmpCommon::contextHeap())
                               Filter(new(CmpCommon::contextHeap())
                                        CutOp(1, CmpCommon::contextHeap()),
                                      CmpCommon::contextHeap()),
                             CmpCommon::contextHeap()));
  set->insert(r);

  set->enable(r->getNumber(),
                        set->getSecondPassNumber());

  JoinLeftShiftRuleNumber = r->getNumber();

  r = new (CmpCommon::contextHeap())
          IndexJoinRule1
             ("Transform a scan into index joins (pass 1)",
              new (CmpCommon::contextHeap())
		Scan( REL_SCAN, CmpCommon::contextHeap() ),
              new (CmpCommon::contextHeap())
              Join(new (CmpCommon::contextHeap())
                     Scan(REL_SCAN, CmpCommon::contextHeap()),
                   new (CmpCommon::contextHeap())
                     Scan(REL_SCAN, CmpCommon::contextHeap()),
                   REL_JOIN,
                   NULL,
                   FALSE,
                   FALSE,
                   CmpCommon::contextHeap()));
  set->insert(r);
  set->enable (r->getNumber(),
                         set->getFirstPassNumber());

  r = new (CmpCommon::contextHeap())
          IndexJoinRule2
             ("Transform a scan into index joins (pass 2)",
              new (CmpCommon::contextHeap())
                Scan(REL_SCAN, CmpCommon::contextHeap()),
              new (CmpCommon::contextHeap())
                Join( new(CmpCommon::contextHeap())
                        Scan(REL_SCAN, CmpCommon::contextHeap()),
                      new(CmpCommon::contextHeap())
                        Scan(REL_SCAN, CmpCommon::contextHeap()),
                      REL_JOIN,
                      NULL,
                      FALSE,
                      FALSE,
                      CmpCommon::contextHeap()));
  set->insert(r);
  set->enable (r->getNumber(),
                         set->getSecondPassNumber());
  r = new(CmpCommon::contextHeap())
         OrOptimizationRule ("Transform scan to union for OR optimization",
			     new (CmpCommon::contextHeap())
			       Scan( REL_SCAN, CmpCommon::contextHeap() ),
			     new (CmpCommon::contextHeap())
			       MapValueIds(
                                 new (CmpCommon::contextHeap())
			           Union(
                                     new(CmpCommon::contextHeap())
				       Scan(REL_SCAN,
					    CmpCommon::contextHeap()),
                                     new(CmpCommon::contextHeap())
                                       Scan(REL_SCAN,
					    CmpCommon::contextHeap()),
				     NULL,
				     NULL,
				     REL_UNION,
				     CmpCommon::contextHeap()),
				 CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber(),
                        set->getSecondPassNumber());

  r = new(CmpCommon::contextHeap()) RoutineJoinToTSJRule
    ("RoutineJoin to TSJ Rule",
     new(CmpCommon::contextHeap())
     Join(new(CmpCommon::contextHeap())
            CutOp(0, CmpCommon::contextHeap()),
          new(CmpCommon::contextHeap())
            CutOp(1, CmpCommon::contextHeap()),
          REL_ROUTINE_JOIN,
          NULL,
          FALSE,
          FALSE,
          CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
     Join(new(CmpCommon::contextHeap())
            CutOp(0, CmpCommon::contextHeap()),
          new(CmpCommon::contextHeap())
            Filter (new (CmpCommon::contextHeap())
                      CutOp(1, CmpCommon::contextHeap()),
                    CmpCommon::contextHeap()),
          REL_TSJ, 
          NULL, 
          FALSE, 
          FALSE, 
          CmpCommon::contextHeap() ));
  set->insert(r);
                        
  set->enable(r->getNumber());

  RoutineJoinToTSJRuleNumber = r->getNumber();

  r = new (CmpCommon::contextHeap())
          JoinToTSJRule
             ("Transform Join to TSJ",
              new (CmpCommon::contextHeap())
              WildCardOp (REL_ANY_NON_TSJ_JOIN,
                          0,
                          new (CmpCommon::contextHeap())
                            CutOp(0, CmpCommon::contextHeap()),
                          new (CmpCommon::contextHeap())
                            CutOp(1, CmpCommon::contextHeap()),
                          CmpCommon::contextHeap()),
              new (CmpCommon::contextHeap())
                  Join (new (CmpCommon::contextHeap())
                          CutOp(0, CmpCommon::contextHeap()),
                        new (CmpCommon::contextHeap())
                          Filter (new (CmpCommon::contextHeap())
                                    CutOp(1, CmpCommon::contextHeap()),
                                  CmpCommon::contextHeap()),
                        REL_JOIN,
                        NULL,
                        FALSE,
                        FALSE,
                        CmpCommon::contextHeap()));
  set->insert(r);
  // xxx need to make this first pass after we remove the special
  // code for Anti-semi join and embedded deletes. For now its already
  // enabled in pass 1 at start of optimization unless comp_bool_71 is ON
  set->enable (r->getNumber(),
                         set->getSecondPassNumber());

  JoinToTSJRuleNumber = r->getNumber();

  r = new (CmpCommon::contextHeap())
          TSJFlowRule
             ("Transform GenericUpdate to TSJFlow expression",
              new (CmpCommon::contextHeap())
                  WildCardOp
                     (REL_ANY_UNARY_GEN_UPDATE,
                      0,
                      new (CmpCommon::contextHeap())
                        CutOp(0, CmpCommon::contextHeap()),
                      NULL,
                      CmpCommon::contextHeap()),
              new (CmpCommon::contextHeap())
                  Join (new (CmpCommon::contextHeap())
                          CutOp(0, CmpCommon::contextHeap()),
                        new (CmpCommon::contextHeap())
                            WildCardOp (REL_ANY_LEAF_GEN_UPDATE,
                                        0,
                                        NULL,
                                        NULL,
                                        CmpCommon::contextHeap()),
                        REL_TSJ_FLOW,
                        NULL,
                        FALSE,
                        FALSE,
                        CmpCommon::contextHeap()));
  set->insert(r);
  set->enable (r->getNumber());

  // Raj P - 10/2000
  // Rule to fire for SPJ CALL statement
  r = new (CmpCommon::contextHeap())
          TSJUDRRule
             ("Transform CALL nodes to TSJUDR expression",
              new (CmpCommon::contextHeap())
                  WildCardOp
                     (REL_ANY_UNARY_GEN_UPDATE,
                      0,
                      new (CmpCommon::contextHeap())
                        CutOp(0, CmpCommon::contextHeap()),
                      NULL,
                      CmpCommon::contextHeap()),
              new (CmpCommon::contextHeap())
                  Join (new (CmpCommon::contextHeap())
                          CutOp(0, CmpCommon::contextHeap()),
                        new (CmpCommon::contextHeap())
                            WildCardOp (REL_ANY_LEAF_GEN_UPDATE,
                                        0,
                                        NULL,
                                        NULL,
                                        CmpCommon::contextHeap()),
                        REL_TSJ_FLOW,
                        NULL,
                        FALSE,
                        FALSE,
                        CmpCommon::contextHeap()));
  set->insert(r);
  set->enable (r->getNumber());

  r = new (CmpCommon::contextHeap())
          TSJRule
             ("Transform GenericUpdate to TSJ expression",
              new (CmpCommon::contextHeap())
                  WildCardOp
                     (REL_ANY_UNARY_GEN_UPDATE,
                      0,
                      new (CmpCommon::contextHeap())
                        CutOp(0, CmpCommon::contextHeap()),
                      NULL,
                      CmpCommon::contextHeap()),
              new (CmpCommon::contextHeap())
                  Join (new (CmpCommon::contextHeap())
                          CutOp(0, CmpCommon::contextHeap()),
                        new (CmpCommon::contextHeap())
                            WildCardOp (REL_ANY_LEAF_GEN_UPDATE,
                                        0,
                                        NULL,
                                        NULL,
                                        CmpCommon::contextHeap()),
                        REL_TSJ,
                        NULL,
                        FALSE,
                        FALSE,
                        CmpCommon::contextHeap()));
  set->insert(r);
  set->enable (r->getNumber());

  r = new (CmpCommon::contextHeap())
          FilterRule0
             ("Transform Filter on a leaf",
              new (CmpCommon::contextHeap())
                  Filter(new (CmpCommon::contextHeap())
                             WildCardOp(REL_ANY_LEAF_OP,
                                        0,
                                        NULL,
                                        NULL,
                                        CmpCommon::contextHeap()),
                         CmpCommon::contextHeap()),
              new (CmpCommon::contextHeap())
                   WildCardOp(REL_ANY_LEAF_OP,
                              0,
                              NULL,
                              NULL,
                              CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  FilterRule0Number = r->getNumber();

  r = new (CmpCommon::contextHeap())
          FilterRule1
             ("Transform Filter on a unary op",
              new (CmpCommon::contextHeap())
                  Filter(new (CmpCommon::contextHeap())
                             WildCardOp(REL_ANY_UNARY_OP,
                                        0,
                                        new(CmpCommon::contextHeap())
                                          CutOp(0, CmpCommon::contextHeap()),
                                        NULL,
                                        CmpCommon::contextHeap()),
                         CmpCommon::contextHeap()),
              new (CmpCommon::contextHeap())
                  WildCardOp(REL_ANY_UNARY_OP,
                             0,
                             new (CmpCommon::contextHeap())
                                Filter(new(CmpCommon::contextHeap())
                                         CutOp(0, CmpCommon::contextHeap()),
                                       CmpCommon::contextHeap()),
                             NULL,
                             CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  FilterRule1Number = r->getNumber();

  r = new (CmpCommon::contextHeap())
          FilterRule2
             ("Transform Filter on a binary op",
              new (CmpCommon::contextHeap())
                  Filter(new (CmpCommon::contextHeap())
                             WildCardOp(REL_ANY_BINARY_OP,
                                        0,
                                        new (CmpCommon::contextHeap())
                                          CutOp(0, CmpCommon::contextHeap()),
                                        new (CmpCommon::contextHeap())
                                          CutOp(1, CmpCommon::contextHeap()),
                                        CmpCommon::contextHeap()),
                         CmpCommon::contextHeap()),
              new (CmpCommon::contextHeap())
                  WildCardOp(REL_ANY_BINARY_OP,
                             0,
                             new (CmpCommon::contextHeap())
                                 Filter(new (CmpCommon::contextHeap())
                                          CutOp(0, CmpCommon::contextHeap()),
                                        CmpCommon::contextHeap()),
                             new (CmpCommon::contextHeap())
                                 Filter(new (CmpCommon::contextHeap())
                                          CutOp(1, CmpCommon::contextHeap()),
                                        CmpCommon::contextHeap()),
                             CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  FilterRule2Number = r->getNumber();

  r = new (CmpCommon::contextHeap())
          GroupByEliminationRule
             ("Eliminate unnecessary groupbys",
              new (CmpCommon::contextHeap())
                   GroupByAgg(new (CmpCommon::contextHeap())
                                CutOp(0, CmpCommon::contextHeap()),
                              REL_GROUPBY,
                              NULL,
                              NULL,
                              CmpCommon::contextHeap()),
              new (CmpCommon::contextHeap())
                CutOp(0, CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber(),
                         set->getFirstPassNumber());

  r = new (CmpCommon::contextHeap())
          GroupByMVQRRule
             ("MVQR GroupBy Rewrite Rule",
     // pattern 
      new (CmpCommon::contextHeap())
          WildCardOp(REL_ANY_GROUP,
                     1,
                     new (CmpCommon::contextHeap())
                       CutOp(0, CmpCommon::contextHeap()),
                     NULL,
                     CmpCommon::contextHeap()),
      // substitute
      new (CmpCommon::contextHeap())
          MapValueIds(new(CmpCommon::contextHeap())
                        CutOp(0, CmpCommon::contextHeap())));

  set->insert(r);
  set->enable (r->getNumber(),
                         set->getSecondPassNumber());

  r = new (CmpCommon::contextHeap())
          GroupBySplitRule
             ("Split a groupby",
              new (CmpCommon::contextHeap())
                  WildCardOp(REL_ANY_GROUP,
                             1,
                             new (CmpCommon::contextHeap())
                               CutOp(0, CmpCommon::contextHeap()),
                             NULL,
                             CmpCommon::contextHeap()),
              new (CmpCommon::contextHeap())
                  MapValueIds(new (CmpCommon::contextHeap())
                               WildCardOp(REL_ANY_GROUP,
                                          1,
                                          new (CmpCommon::contextHeap())
                                            GroupByAgg( new(CmpCommon::contextHeap())
                                                          CutOp(0, CmpCommon::contextHeap()),
                                                        REL_GROUPBY,
                                                        NULL,
                                                        NULL,
                                                        CmpCommon::contextHeap()),
                                          NULL,
                                          CmpCommon::contextHeap()),
                              CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber(),
                        set->getFirstPassNumber());

  r = new (CmpCommon::contextHeap())
          AggrDistinctEliminationRule
             ("Eliminate aggregate distinct rule",
              new (CmpCommon::contextHeap())
                  WildCardOp(REL_ANY_GROUP,
                             1,
                             new (CmpCommon::contextHeap())
                               CutOp(0, CmpCommon::contextHeap()),
                             NULL,
                             CmpCommon::contextHeap()),
              new (CmpCommon::contextHeap())
                   MapValueIds(new(CmpCommon::contextHeap())
                                 WildCardOp(REL_ANY_GROUP,
                                            1,
                                            new (CmpCommon::contextHeap())
                                            GroupByAgg(new (CmpCommon::contextHeap())
                                                         CutOp(0, CmpCommon::contextHeap()),
                                                       REL_GROUPBY,
                                                       NULL,
                                                       NULL,
                                                       CmpCommon::contextHeap()),
                                            NULL,
                                            CmpCommon::contextHeap()),
                               CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

  r = new (CmpCommon::contextHeap())
          GroupByTernarySplitRule
             ("Split a partial groupby that is a leaf",
              new (CmpCommon::contextHeap())
                  WildCardOp(REL_ANY_GROUP,
                             1,
                             new (CmpCommon::contextHeap())
                               CutOp(0, CmpCommon::contextHeap()),
                             NULL,
                             CmpCommon::contextHeap()),
              new (CmpCommon::contextHeap())
                  MapValueIds(new (CmpCommon::contextHeap())
                                  WildCardOp(REL_ANY_GROUP,
                                             1,
                                             new (CmpCommon::contextHeap())
                                             GroupByAgg(new(CmpCommon::contextHeap())
                                                          CutOp(0, CmpCommon::contextHeap()),
                                                        REL_GROUPBY,
                                                        NULL,
                                                        NULL,
                                                        CmpCommon::contextHeap()),
                                             NULL,
                                             CmpCommon::contextHeap()),
                              CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber(),
                        set->getSecondPassNumber());

  r = new (CmpCommon::contextHeap())
          GroupByOnJoinRule
             ("Push groupby down past a join",
              new (CmpCommon::contextHeap())
                  GroupByAgg(new (CmpCommon::contextHeap())
                               WildCardOp(REL_ANY_INNER_JOIN,
                                          1,
                                          new (CmpCommon::contextHeap())
                                            CutOp(0, CmpCommon::contextHeap()),
                                          new (CmpCommon::contextHeap())
                                            CutOp(1, CmpCommon::contextHeap()),
                                          CmpCommon::contextHeap()),
                             REL_GROUPBY,
                             NULL,
                             NULL,
                             CmpCommon::contextHeap()),
              new (CmpCommon::contextHeap())
                WildCardOp(REL_ANY_INNER_JOIN,
                           1,
                           new (CmpCommon::contextHeap())
                           GroupByAgg(new (CmpCommon::contextHeap())
                                        CutOp(0, CmpCommon::contextHeap()),
                                      REL_GROUPBY,
                                      NULL,
                                      NULL,
                                      CmpCommon::contextHeap()),
                           new (CmpCommon::contextHeap())
                             CutOp(1, CmpCommon::contextHeap()),
                           CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber(),
                        set->getSecondPassNumber());

  r = new (CmpCommon::contextHeap())
          PartialGroupByOnTSJRule
             ("Push partial groupby down past a tsj",
              new (CmpCommon::contextHeap())
                GroupByAgg(new (CmpCommon::contextHeap())
                             WildCardOp(REL_ANY_TSJ,
                                        1,
                                        new (CmpCommon::contextHeap())
                                          CutOp(0, CmpCommon::contextHeap()),
                                        new (CmpCommon::contextHeap())
                                          CutOp(1, CmpCommon::contextHeap()),
                                        CmpCommon::contextHeap()),
                           REL_GROUPBY,
                           NULL,
                           NULL,
                           CmpCommon::contextHeap()),
              new (CmpCommon::contextHeap())
                  WildCardOp(REL_ANY_TSJ,
                             1,
                             new (CmpCommon::contextHeap())
                                 GroupByAgg(new (CmpCommon::contextHeap())
                                              CutOp(0, CmpCommon::contextHeap()),
                                            REL_GROUPBY,
                                            NULL,
                                            NULL,
                                            CmpCommon::contextHeap()),
                             new (CmpCommon::contextHeap())
                               CutOp(1, CmpCommon::contextHeap()),
                             CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber(),
                        set->getSecondPassNumber());

  r = new(CmpCommon::contextHeap()) ShortCutGroupByRule
   ("Transform anytrue subquery groupby or min/max aggr to shortcut groupby",
     new(CmpCommon::contextHeap())
       WildCardOp(REL_ANY_GROUP,
                  0,
                  new(CmpCommon::contextHeap())
                    CutOp(0, CmpCommon::contextHeap()),
                  NULL,
                  CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
       ShortCutGroupBy(new(CmpCommon::contextHeap())
                        CutOp(0, CmpCommon::contextHeap()),
                      REL_SHORTCUT_GROUPBY,
                      NULL,
                      NULL,
                      CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber(),
                        set->getSecondPassNumber());

  r = new(CmpCommon::contextHeap()) CommonSubExprRule
   ("Eliminate any CommonSubExpr nodes left from the normalizer - for now",
    new(CmpCommon::contextHeap())
      CommonSubExprRef(new(CmpCommon::contextHeap())
                         CutOp(0, CmpCommon::contextHeap()),
                       "",
                       CmpCommon::contextHeap()),
    new(CmpCommon::contextHeap())
      CutOp(0, CmpCommon::contextHeap()));
  set->insert(r);
  set->enable(r->getNumber());

   r = new (CmpCommon::contextHeap()) SampleScanRule
             ("Transform RelSample above a Scan",
              new (CmpCommon::contextHeap())
                  RelSample(new(CmpCommon::contextHeap())
                             Scan(REL_SCAN, CmpCommon::contextHeap()),
                             RelSample::ANY,
                             NULL,
                             NULL,
                             CmpCommon::contextHeap()),
              new (CmpCommon::contextHeap())
                  Scan(REL_SCAN, CmpCommon::contextHeap()));

  set->insert(r);
  set->enable(r->getNumber());

//++MV,
  r = new(CmpCommon::contextHeap())
        JoinToBushyTreeRule("Join to bush tree",
              new (CmpCommon::contextHeap())
                   WildCardOp(REL_ANY_INNER_JOIN,
			      0,
			      new (CmpCommon::contextHeap())
			          WildCardOp(REL_ANY_INNER_JOIN,
					     1,
					     new (CmpCommon::contextHeap())
					       CutOp(0,
						     CmpCommon::contextHeap()),
					     new (CmpCommon::contextHeap())
					       CutOp(1, CmpCommon::contextHeap()),
					     CmpCommon::contextHeap()),
			      new (CmpCommon::contextHeap())
			        CutOp(2, CmpCommon::contextHeap()),
			      CmpCommon::contextHeap()),
	      new (CmpCommon::contextHeap())
                  WildCardOp(REL_ANY_INNER_JOIN,
			     0,
			     new (CmpCommon::contextHeap())
			        CutOp(0, CmpCommon::contextHeap()),
			     new (CmpCommon::contextHeap())
			       WildCardOp(REL_ANY_INNER_JOIN,
					  1,
					  new(CmpCommon::contextHeap())
					    CutOp(1, CmpCommon::contextHeap()),
					  new(CmpCommon::contextHeap())
					    CutOp(2, CmpCommon::contextHeap()),
					  CmpCommon::contextHeap()),
			     CmpCommon::contextHeap()));

  set->insert(r);
  set->enable(r->getNumber(),
			set->getSecondPassNumber());

/*
  r = new(CmpCommon::contextHeap()) HbaseScanRule
    ("Hbase Scan transformation rule",
     new(CmpCommon::contextHeap()) Scan(REL_SCAN, CmpCommon::contextHeap()),
     new(CmpCommon::contextHeap())
     HbaseAccess(CmpCommon::contextHeap()));

  set->insert(r);
  set->enable(r->getNumber());
*/

//*****
//--MV
}

// -----------------------------------------------------------------------
// methods for transformation rules
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// methods for JoinCommutativityRule
// -----------------------------------------------------------------------

NABoolean JoinCommutativityRule::topMatch (RelExpr * expr,
                                           Context * context)
{
  // check if this rule has been disabled via RuleGuidanceCQD
  // the CQD is COMP_INT_77 and it represents a bitmap
  // below we check if the bit # 1 is ON
  if(CURRSTMT_OPTDEFAULTS->isRuleDisabled(1))
    return FALSE;

  // if we want the specific join order given by the normalizer
  // then lets not waste our time doing this
  if (CURRSTMT_OPTDEFAULTS->joinOrderByUser())
    return FALSE;

  if (CURRSTMT_CQSWA && CURRSTMT_CQSWA->reArrangementSuccessful_)
    return FALSE;

  //++MV, do not apply to join on top of log insert before JoinToBushyTreeRule
  if (expr->getInliningInfo().isDrivingMvLogInsert())
    return FALSE;

  //++MV, do not apply to this join
  if (expr->getInliningInfo().isJoinOrderForcedByInlining())
    return FALSE;

  // if the rule doesn't match the general pattern or if this is a
  // semi, antisemi or left join. quit
  if (NOT Rule::topMatch(expr,context))
    return FALSE;

  // QSTUFF
  CMPASSERT (NOT ((Join *)expr)->child(1).getGroupAttr()->isStream());
  CMPASSERT (NOT ((Join *)expr)->child(1).getGroupAttr()->isEmbeddedUpdateOrDelete());

  if (((Join *)expr)->child(0).getGroupAttr()->isStream()){
    return FALSE;
  }
  if  (((Join *)expr)->child(0).getGroupAttr()->isEmbeddedUpdateOrDelete()){
    return FALSE;
  }
  // QSTUFF

  // Semi Joins cannot be left-shifted
  if (NOT ((Join *)expr)->isInnerNonSemiJoin())
    return FALSE;

  // The join commutativity rule will succeed, if both its
  // children are nodes other than inner join nodes. Since the
  // children to the join nodes in this rule are cut nodes, test
  // the "numJoinedTables" attribute of the logical properties instead.
  if (expr->getGroupAttr()->getNumJoinedTables() == 2)
    return TRUE;

  // If zigzag CQD is off and this is not a join resulted from the PTRule
  // Then we do not apply the rule.
  if (CURRSTMT_OPTDEFAULTS->isZigZagTreeConsidered() == DF_OFF &&
      NOT ((Join *)expr)->isJoinFromPTRule())
    return FALSE;

  // If this join is from MJPrimeTableRule then we allow all its zigzag
  // variations regardless of the values of the zigzag CQD. We however
  // limit that to join subtrees of 16 child or less
  if (((Join *)expr)->isJoinFromPTRule() &&
      expr->getGroupAttr()->getNumJoinedTables() > 16)
    return FALSE;

  else if (CURRSTMT_OPTDEFAULTS->isZigZagTreeConsidered() == DF_SYSTEM)
    {
      //----------------ZZT control------------------//
      CostScalar r0Max =
        expr->child(0).getGroupAttr()->getResultMaxCardinalityForEmptyInput()
        * expr->child(0).getGroupAttr()->getRecordLength();

      CostScalar r1 =
        expr->child(1).getGroupAttr()->getResultCardinalityForEmptyInput()
        * expr->child(1).getGroupAttr()->getRecordLength();

      if (r0Max >= r1)
        return FALSE;

      // If r1Max can fit in memory then in such case no big advantage of
      // switching the children of the hash join
      CostScalar r1Max =
        expr->child(1).getGroupAttr()->getResultMaxCardinalityForEmptyInput()
        * expr->child(1).getGroupAttr()->getRecordLength();
      if( r1Max < CostScalar(1024* CURRSTMT_OPTDEFAULTS->getMemoryLimitPerCPU()))
        return FALSE;
      // we could go more aggressive and say we should have
      // r0 < 1024* CURRSTMT_OPTDEFAULTS->getMemoryLimitPerCPU() < r1
      // to allow zigzag. We may also want to pay attention
      // to ordered hash join now (although zigzag was initially
      // proposed for Hybrid only).
    }

  return TRUE;
}

RelExpr * JoinCommutativityRule::nextSubstitute(RelExpr * before,
                                                Context *,
                                                RuleSubstituteMemory * &memory)
{

  Join *result = (Join *) Rule::nextSubstitute(before,NULL,memory);

  // don't try to apply the commutativity rule on the result, it would
  // just give the original expression back
  // NOTE: Guidance could be used to do this, but that would have the
  // disadvantage that when a group gets optimized twice (e.g. for two
  // different reqd. orders), the commutativity rule would be applied
  // unnecessarily during the second optimization, because the guidance
  // object was active only during the first optimization.
  result->contextInsensRules() += getNumber();

  // synthesize the estimated logical properties for the new node
  result->flipChildren();

  // If this yielded a bushy tree do not transform it anymore
  // we only want to do zig-zag trees. We do not want to do
  // TSJs on the any generated bushy tree either.
  if (before->getGroupAttr()->getNumJoinedTables() != 2)
  {
    result->contextInsensRules() += GlobalRuleSet->transformationRules();
    result->setTransformComplete();

    // we're an optional zigzag join
    result->setJoinForZigZag();

    // Disable also the merge join rule if this is a join where the
    // rows from one of the children will have at most one match
    // in the other.
    // Those types of merge joins are symmetric in the executor.
     if (result->eitherChildHasUniqueMatch())
       result->contextInsensRules() += MergeJoinRuleNumber;
  }

  // If the before join resulted from a application of MJPrimeTableRule, directly
  // or indirectly, we set the result join to be also from MJPrimeTableRule.
  if (((Join*)before)->isJoinFromPTRule())
    result->setJoinFromPTRule();

  result->setPotential(before->getPotential());
  
  return result;
}

// -----------------------------------------------------------------------
// methods for JoinLeftShiftRule
// -----------------------------------------------------------------------

NABoolean JoinLeftShiftRule::topMatch (RelExpr * expr,
                                       Context * context)
{
  // check if this rule has been disabled via RuleGuidanceCQD
  // the CQD is COMP_INT_77 and it represents a bitmap
  // below we check if the bit # 2 is ON
  if(CURRSTMT_OPTDEFAULTS->isRuleDisabled(2))
    return FALSE;

  // if we want the specific join order given by the normalizer
  // then lets not waste our time doing this
  if (CURRSTMT_OPTDEFAULTS->joinOrderByUser())
    return FALSE;

  // For now if MultiJoin rewrite take place, the left shift rule is disabled.
  if (QueryAnalysis::Instance() && QueryAnalysis::Instance()->multiJoinsUsed())
    return FALSE;

  if (CURRSTMT_CQSWA && CURRSTMT_CQSWA->reArrangementSuccessful_)
    return FALSE;

  //++MV, do not apply to join on top of log insert before JoinToBushyTreeRule
  if (expr->getInliningInfo().isDrivingMvLogInsert())
    return FALSE;

  if (NOT Rule::topMatch(expr,context))
    return FALSE;

  // QSTUFF
  CMPASSERT (NOT ((Join *)expr)->child(1).getGroupAttr()->isStream());
  CMPASSERT (NOT ((Join *)expr)->child(1).getGroupAttr()->isEmbeddedUpdateOrDelete());
  // QSTUFF

  // don't left shift anti-semi join.
  if (((Join *)expr)->isAntiSemiJoin())
    return FALSE;

  // QSTUFF
  // we don't allow generic update roots to be moved
  if (((Join *)expr)->getGroupAttr()->isGenericUpdateRoot() ||
      ((Join *)expr)->child(0).getGroupAttr()->isGenericUpdateRoot())
    return FALSE;
  // QSTUFF

  // When looking at the top node of a possible match for this rule,
  // return FALSE, if the left input group doesn't contain a
  // logical expression that is an inner join of at least 2 tables
  return (expr->child(0).getGroupAttr()->getNumJoinedTables() >= 2);
}

RelExpr * JoinLeftShiftRule::nextSubstitute(RelExpr * before,
                                            Context * context/*context*/,
                                            RuleSubstituteMemory * & memory)
{

  // Check to see whether any transformations should be applied on this
  // join child.
  if (((Join *)(before->child(0).getPtr()))->isTransformComplete()) return NULL;

  // do the default pattern substitution logic
  Join *result = (Join *)Rule::nextSubstitute(before,NULL,memory);
  Join *joinChild = (Join *)result->child(0).getPtr();

  // Move all selection predicates to the new top join
  result->selectionPred() += joinChild->selectionPred();
  joinChild->selectionPred().clear();

  // Allocate a new Group Attributes for the child.
  joinChild->setGroupAttr(new (CmpCommon::statementHeap()) GroupAttributes);

  // Compute the set of values that each child will potentially require
  // for evaluating expressions. Also compute the set of values that
  // the child is capable of producing as output.
  // This call will NOT effect the characteristic input and output
  // values of CutOps.
  result->allocateAndPrimeGroupAttributes();

  // If the right child does not have any predicates on its filter,
  // eliminate the filter.
  Filter * filterPtr = (Filter *)result->child(1).getPtr();

  // For the case where joinChild is a semiJoin, leftJoin or an
  // antiSemiJoin make sure that the joinPred does not reference
  // anything from result->child(1)
  // i.e. joinPred is covered by joinChild inputs and its childs outputs
  if (NOT joinChild->getJoinPred().isEmpty())
    {
      ValueIdSet charInputs =
        joinChild->getGroupAttr()->getCharacteristicInputs();
      ValueIdSet child0Outputs =
        joinChild->child(0)->getGroupAttr()->getCharacteristicOutputs();

      ValueIdSet availableExprs = charInputs;
      availableExprs += child0Outputs;
      availableExprs +=
        joinChild->child(1)->getGroupAttr()->getCharacteristicOutputs();

      ValueIdSet joinPredicates = joinChild->getJoinPred();
      ValueIdSet vegPredicates;

      // Separate the vegPredicates from the rest
      joinPredicates.lookForVEGPredicates(vegPredicates);
      joinPredicates -= vegPredicates;

      // make sure we can cover the join predicates
      joinPredicates.removeCoveredExprs(availableExprs);

      //   The Veg predicates must be covered by both children.
      //   We know that joinChild->child(1) covers them, verify
      // that the other child also covers them.
      //   We used to call removeCoveredExprs on the vegPredicates, just
      // like we do above for the join predicates. But, the method
      // removeCoveredExprs doesn't look for coverage of the vegPredicates,
      // it looks for coverage of their underlying vegRefs. The coverage
      // code for vegRefs returns TRUE if any component of the vegRef is
      // is "covered", including a constant. We don't want to say a
      // vegPredicate is "covered" if it references a constant, we only
      // want to say it is "covered" if it references a characteristic
      // output of child0. Otherwise, we can erroneously think that this
      // left shift is OK in some cases and we can then get wrong answers.
      //   For example:
      // SELECT t0.i1 from j3 t0, j1 t2 left join j3 t1 on t2.i1 = 1;
      //   If the initial join order is t0,t2,t1, we don't want to allow
      // t1 to be left shifted with t2. But removeCoveredExpr would think
      // that the vegPredicate "t2.i1 = 1" was "covered" by child0 (t0),
      // because the constant "1" is always "covered". But this is wrong.
      //   It turns out that VEGPredicate::isCovered does exactly what needs
      // to be done, so we now call it instead.
      ValueIdSet dummyReferencedInputs,dummyCoveredExpr,dummyUncoveredExpr;
      GroupAttributes emptyGA;
      emptyGA.setCharacteristicOutputs(child0Outputs);
      NABoolean vegPredsCovered =
        vegPredicates.isCovered(charInputs,
                                emptyGA,
                                dummyReferencedInputs,
                                dummyCoveredExpr,
                                dummyUncoveredExpr);


      if (NOT vegPredsCovered OR
          NOT joinPredicates.isEmpty())
      {
        joinChild->deleteInstance();
        result->deleteInstance();
        filterPtr->deleteInstance();
        return NULL;
      }
    }

  // Push down as many full or partial expressions that can be
  // computed by the children. Recompute the Group Attributes of
  // each child that is not a CutOp.
  result->pushdownCoveredExpr
            (result->getGroupAttr()->getCharacteristicOutputs(),
             result->getGroupAttr()->getCharacteristicInputs(),
             result->selectionPred());

  if (filterPtr->selectionPred().isEmpty())
    {
      result->child(1) = filterPtr->child(0).getPtr();
      filterPtr->deleteInstance();        // delete the Filter
      filterPtr=NULL;
    }
  else
    {
      // Check to see if the filter node is adding any new characteristic
      // inputs (i.e. outer references).  If not, no true join predicates were
      // pushed down.  So, eliminate this filter.
      if( (filterPtr->getGroupAttr()->getCharacteristicInputs() ==
            filterPtr->child(0).getGroupAttr()->getCharacteristicInputs()) AND
          (filterPtr->getGroupAttr()->getCharacteristicOutputs() ==
            filterPtr->child(0).getGroupAttr()->getCharacteristicOutputs()) )
      {
        result->child(1) = filterPtr->child(0).getPtr();
        filterPtr->deleteInstance();
        filterPtr=NULL;
      }
      else
      {
        filterPtr->synthLogProp();
        filterPtr->getGroupAttr()->addToAvailableBtreeIndexes(
          filterPtr->child(0).getGroupAttr()->getAvailableBtreeIndexes());
      }
    }

  // Call pushdownCovered expressions on the joinChild
  // Since the children of the joinChild are Cut operators, nothing
  // will be pushed to them but predicates will be removed from
  // the joinChild if it is determined that they were given (givable)
  // to the cut operators before.
  joinChild->pushdownCoveredExpr
            (joinChild->getGroupAttr()->getCharacteristicOutputs(),
             joinChild->getGroupAttr()->getCharacteristicInputs(),
             joinChild->selectionPred());

  // synthesize the estimated logical properties for the new node
  joinChild->synthLogProp();

  // Call synthLogProp on the result also so that we can call
  // findEquiJoinPredicates() with the new set of predicates
  result->synthLogProp();

  //----------------Heuristics Domain-------------------------------
  //
  //               r3  /                       r3  /
  //                  /                           /
  //                Join                        Join
  //           r1  /   \  rB               r2  /   \   rA
  //              /     \                     /     \
  //            Join           ==>          Join
  //       r0  /   \  rA               r0  /   \  rB
  //          /     \                     /     \
  //

  CostScalar r0 =
    joinChild->child(0).getGroupAttr()->getResultCardinalityForEmptyInput();
  CostScalar r1 =
    before->child(0).getGroupAttr()->getResultCardinalityForEmptyInput();
  CostScalar r2 =
    joinChild->getGroupAttr()->getResultCardinalityForEmptyInput();
  CostScalar rA =
    result->child(1).getGroupAttr()->getResultCardinalityForEmptyInput();
  CostScalar rB =
    joinChild->child(1).getGroupAttr()->getResultCardinalityForEmptyInput();
  CostScalar r3 =
    before->getGroupAttr()->getResultCardinalityForEmptyInput();

  CostScalar s0 = r0 * joinChild->child(0).getGroupAttr()->getRecordLength();
  CostScalar s1 = r1 * before->child(0).getGroupAttr()->getRecordLength();
  CostScalar s2 = r2 * joinChild->getGroupAttr()->getRecordLength();
  CostScalar sA = rA * result->child(1).getGroupAttr()->getRecordLength();
  CostScalar sB = rB * joinChild->child(1).getGroupAttr()->getRecordLength();
  CostScalar s3 = r3 * before->getGroupAttr()->getRecordLength();

  //-------------------TSJ control----------------------------------

  // Nested join control as coded does not work. Causes bug #129
  // (Genesis case #10-000222-6834).  Commenting out. Code also
  // makes no sense.
  /*
  if (CURRSTMT_OPTDEFAULTS->isNestedJoinControlEnabled())
  {

    ValueIdSet beforePred = before->selectionPred();
    beforePred += ((Join*)before)->joinPred();

    ValueIdSet beforeChildPred = (before->child(0).getPtr())->selectionPred();
    beforeChildPred += ((Join*)(before->child(0).getPtr()))->joinPred();

    ValueIdSet resultPred = result->selectionPred();
    resultPred += result->joinPred();

    ValueIdSet resultChildPred = joinChild->selectionPred();
    resultChildPred += joinChild->joinPred();

    if (beforePred == resultChildPred)
    {
      if(r0 >= r1)
        {joinChild->doNotTransformToTSJ();}
      else
        {((Join*)before)->doNotTransformToTSJ();}
    }

    if (resultPred == beforeChildPred)
    {
      if(r0 <= r2)
        {result->doNotTransformToTSJ();}
      else
        {((Join*)(before->child(0).getPtr()))->doNotTransformToTSJ();}
    }
  }
  */

  //-------------------------------------------------------------

  //------------- Minimum Flow Heuristic -------------
  if (CURRSTMT_OPTDEFAULTS->optimizerHeuristic5())
  {
    {
      // This part of the flow did not change.
      CostScalar constFlow = s0 + s3 + sA + sB;
      CostScalar fudgeFactor = 1.5;
      if( (constFlow+s2) >= (fudgeFactor*(constFlow+s1)+1000) )
      // maybe also add (AND r2 > r1 * fudg factor) cuz in some cases
      // row count could be even more important than size e.g TSJ (#probes)
      {
        result->contextInsensRules() += GlobalRuleSet->implementationRules();
      }
      else if( (constFlow+s1) > (fudgeFactor*(constFlow+s2)+1000) )
      {
        ((Join*)before)->contextInsensRules() += GlobalRuleSet->implementationRules();
      }
    }
  }
  //------------------------------------------------------------

  //-------------------X product reduction----------------------
  // allow cross product control if:
  // 1) cross_product_control is active AND
  // 2) multijoin has >1 base table
  NABoolean allowCrossProductControl =
    (CURRSTMT_OPTDEFAULTS->isCrossProductControlEnabled()) AND
    (before->getGroupAttr()->getGroupAnalysis()->
     getAllSubtreeTables().entries() > 1);
  if (allowCrossProductControl)
  {
    if (NOT ((Join*)(before->child(0).getPtr()))->isCrossProduct()
      AND joinChild->isCrossProduct()
      AND NOT ((Join*)before)->isCrossProduct())
    {
      if (r2 > CostScalar(10) * r1)
      {
        result->contextInsensRules() += GlobalRuleSet->transformationRules();
        result->contextInsensRules() += GlobalRuleSet->implementationRules();
        joinChild->contextInsensRules() += GlobalRuleSet->transformationRules();
        joinChild->contextInsensRules() += GlobalRuleSet->implementationRules();
      }
    }

    if (result->isCrossProduct()
      AND joinChild->isCrossProduct())
    {
      if (r2 >= r1)
      {
        result->contextInsensRules() += GlobalRuleSet->implementationRules();
      }
    }
  }
  //------------------------------------------------------------------

  // don't try to apply the left shift rule on the result, it would
  // just give the original expression back
  result->contextInsensRules() += getNumber();

  return (RelExpr *)result;
}

Guidance * JoinLeftShiftRule::guidanceForExploringChild(Guidance *,
                                                        Context *,
                                                        Lng32)
{
  return new(CmpCommon::statementHeap())
             OnceGuidance(JoinToTSJRuleNumber,
             CmpCommon::statementHeap());
}

Guidance * JoinLeftShiftRule::guidanceForExploringSubstitute(Guidance *)
{
  return new (CmpCommon::statementHeap())
    OnceGuidance(getNumber(),CmpCommon::statementHeap());
}

Guidance * JoinLeftShiftRule::guidanceForOptimizingSubstitute(Guidance *,
                                                              Context *)
{
  return new (CmpCommon::statementHeap())
    OnceGuidance(getNumber(),CmpCommon::statementHeap());
}

// -----------------------------------------------------------------------
// methods for IndexJoinRule1
// -----------------------------------------------------------------------
NABoolean IndexJoinRule1::topMatch (RelExpr * expr,
                                    Context * context)
{
  if (NOT Rule::topMatch(expr, context))
    return FALSE;

  Scan * s = (Scan *) expr;


  // Disable the rule for sampleScan for now.
  if (s->isSampleScan() == TRUE)
    return FALSE;
  // go through all indexes and find out in which way they could
  // be used, if this is not already done
  s->addIndexInfo();

  // in pass 1, consider only one index join
  return (s->getNumIndexJoins() == 0 AND
          s->getIndexInfo().entries() > 0);
}

RelExpr * IndexJoinRule1::nextSubstitute(
     RelExpr * before,
     Context * /*context*/,
     RuleSubstituteMemory * & memory)
{
  return nextSubstituteForPass(before,memory,
                               RuleSet::getFirstPassNumber());
}

RelExpr * IndexJoinRule1::nextSubstituteForPass(
     RelExpr * before,
     RuleSubstituteMemory * & memory,
     Lng32 /*pass*/)
{
  RelExpr *result;

  if (memory == NULL)
    {
      // -----------------------------------------------------------------
      // this is the first call, create info on all indexes and create
      // index joins for pass 1
      // -----------------------------------------------------------------

      // allocate a new memory for multiple substitutes
      memory = new (CmpCommon::statementHeap())
        RuleSubstituteMemory(CmpCommon::statementHeap());

     assert(before->getOperatorType() == REL_SCAN);

     // cast the before expression to a scan
     Scan * bef = (Scan *) before;

     CollIndex numIndexJoins = bef->getPossibleIndexJoins().entries();

    ValueIdSet selectionpreds;
    if ((CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON ) &&
	(bef->selectionPred().entries()))
    {
      ValueIdList selectionPredList(bef->selectionPred());
      ItemExpr *inputItemExprTree = selectionPredList.rebuildExprTree(ITM_AND,FALSE,FALSE);
      ItemExpr * resultOld = revertBackToOldTree(CmpCommon::statementHeap(), inputItemExprTree);
      resultOld->convertToValueIdSet(selectionpreds, NULL, ITM_AND);
      doNotReplaceAnItemExpressionForLikePredicates(resultOld,selectionpreds,resultOld);

//     ValueIdSet resultSet;
//	 revertBackToOldTreeUsingValueIdSet(bef->selectionPred(), resultSet);
//	 ItemExpr* resultOld =  resultSet.rebuildExprTree(ITM_AND,FALSE,FALSE);
//     selectionpreds += resultSet;
//	 doNotReplaceAnItemExpressionForLikePredicates(resultOld,selectionpreds,resultOld);
    }
    else
      selectionpreds = bef->selectionPred();

      for (CollIndex i = 0; i < numIndexJoins; i++)
        {
          // get to the recipe on how to build the index join
          ScanIndexInfo *ixi = bef->getPossibleIndexJoins()[i];

          // QSTUFF VV
          // for streams we must check whether all predicates are covered by index
          // and allcharacteristic outputs are produced by index. In case of an
          // embedded update/delete we must use the characteristic outputs of the
          // generic update root. All this ensures that updates to columns covered
          // by predicates or seen by user cause stream to be rescheduled.

          if  (bef->getGroupAttr()->isStream())
          {
            if (ixi->indexPredicates_.contains(selectionpreds)){
              ValueIdSet outputs =
                (bef->getGroupAttr()->isEmbeddedUpdate() ?
                bef->getGroupAttr()->getGenericUpdateRootOutputs() :
              bef->getGroupAttr()->getCharacteristicOutputs());

              // the output value check is only required for embedded
              // updates since a delete always touches all indexes

              if (ixi->outputsFromIndex_.contains(outputs) ||
                (bef->getGroupAttr()->isEmbeddedDelete())) {
                memory->insert(makeSubstituteFromIndexInfo(bef,ixi));
              }
              else {
                *CmpCommon::diags() << DgSqlCode(4207)
                  << DgTableName(ixi->usableIndexes_[0]->getIndexDesc()->
                  getNAFileSet()->getExtFileSetName());
              }
            }
            else{
              *CmpCommon::diags() << DgSqlCode(4208)
                << DgTableName(ixi->usableIndexes_[0]->getIndexDesc()->
                getNAFileSet()->getExtFileSetName());
            }
          }
          else
            // QSTUFF
            // insert the index join into the substitute memory
            memory->insert(makeSubstituteFromIndexInfo(bef,ixi));
        } // for each precomputed index join
   } // memory == NULL

  // ---------------------------------------------------------------------
  // handle case of multiple substitutes
  // ---------------------------------------------------------------------
  if (memory != NULL)
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

RelExpr * IndexJoinRule1::makeSubstituteFromIndexInfo(Scan *bef,
                                                      ScanIndexInfo *ixi)
{  // the substitute is a join between two scan nodes for the same table
  Scan *leftScan = new (CmpCommon::statementHeap())
          Scan(bef->getTableName(),bef->getTableDesc());
  leftScan->setForceIndexInfo();

  Scan *rightScan = new (CmpCommon::statementHeap())
    Scan(bef->getTableName(),bef->getTableDesc());
  rightScan->setForceIndexInfo();
  rightScan->setSuppressHints();

  // propagate SqlTableOpen information pointers
  leftScan->setOptStoi(bef->getOptStoi());
  rightScan->setOptStoi(bef->getOptStoi());

  // hash and merge joins make not much sense for index joins, exclude them
  Join *subs = new (CmpCommon::statementHeap())
    Join(leftScan,rightScan,REL_TSJ);
  subs->setGroupAttr(bef->getGroupAttr());

  // Mark it as an indexJoin
  subs->setIsIndexJoin();

  // propagate access options
  leftScan->accessOptions()  = bef->accessOptions();
  rightScan->accessOptions() = bef->accessOptions();

  // the char. outputs of the left and right scan are already
  // precomputed
  leftScan->setPotentialOutputValues(ixi->outputsFromIndex_);
  rightScan->setPotentialOutputValues(ixi->outputsFromRightScan_);

  // the index predicates go to the left scan
  leftScan->selectionPred() += ixi->indexPredicates_;

  ValueIdSet selectionpreds;
  if ((CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON ) &&
      (bef->selectionPred().entries()))
  {
    ValueIdList selectionPredList(bef->selectionPred());
    ItemExpr *inputItemExprTree = selectionPredList.rebuildExprTree(ITM_AND,FALSE,FALSE);
    ItemExpr * resultOld = revertBackToOldTree(CmpCommon::statementHeap(), inputItemExprTree);
    resultOld->convertToValueIdSet(selectionpreds, NULL, ITM_AND);
	doNotReplaceAnItemExpressionForLikePredicates(resultOld,selectionpreds,resultOld);

//     ValueIdSet resultSet;
//	 revertBackToOldTreeUsingValueIdSet(bef->selectionPred(), resultSet);
//	 ItemExpr* resultOld =  resultSet.rebuildExprTree(ITM_AND,FALSE,FALSE);
//	 selectionpreds += resultSet;
//	 doNotReplaceAnItemExpressionForLikePredicates(resultOld,selectionpreds,resultOld);
  }
  if(CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON &&
     (bef->selectionPred().entries()))
  {
    rightScan->selectionPred() += selectionpreds;
  }
  else
    // all other predicates go to the right child
    rightScan->selectionPred() += bef->selectionPred();
  
  rightScan->selectionPred() += ixi->joinPredicates_;
   // ---------------------------------------------------------------------
  // Bugfix: soln # 10-020930-2072
  // The selPred of the leftScan should be subtracted form the rightScan
  // selPred only if there is not uncovered part in the former.
  // ---------------------------------------------------------------------

  GroupAttributes alwaysCoveredGA;
  ValueIdSet dummyReferencedInputs, alwaysCovered,anyUnCoveredExpr;

  // For coverage check for the characteristics inputs, as well as the
  // index columns

  ValueIdSet availableColumns;
  leftScan->getPotentialOutputValues(availableColumns);
  availableColumns += ixi->indexColumns_;

  availableColumns += bef->getGroupAttr()->getCharacteristicInputs();

  (leftScan->selectionPred()).isCovered(
       availableColumns,
       alwaysCoveredGA,
       dummyReferencedInputs,
       alwaysCovered,
       anyUnCoveredExpr);

  if(anyUnCoveredExpr==NULL)
    rightScan->selectionPred() -= leftScan->selectionPred();

  // ******************************************************************
  // 10-040303-3776: if a predicate factor is covered by the left scan
  // (index scan), no need to re-calculate it on the right child
  // ******************************************************************
  rightScan->selectionPred() -= alwaysCovered;

  // a copy of the join predicates is left on the TSJ, or else
  // the join predicates are executed on the right child alone
  subs->selectionPred() = ixi->joinPredicates_;

  // we also know which indexes the left scan should consider,
  // and we know that all of them are indexOnly for that scan
  leftScan->setIndexOnlyScans(ixi->usableIndexes_);

  //  Only consider the primary access path for the right table!
  // We used to consider any index which can supply the remaining
  // values we need, but then the index join will most likely have to
  // scan the entire right child for every probe, since the alternate
  // index's key is not the primary key columns, and the join predicates
  // will be on the primary key columns.
  //  setIndexOnlyScans demands a set of index descriptors, even if we
  // have only one!
  SET(IndexProperty *) primaryIndex(CmpCommon::statementHeap());
  IndexProperty * ixProp = new(CmpCommon::statementHeap()) IndexProperty(
			  (IndexDesc *)bef->getTableDesc()->getClusteringIndex());
  primaryIndex.insert(ixProp);
  rightScan->setIndexOnlyScans(primaryIndex);

  // set the number of index joins already done for the right scan
  rightScan->setNumIndexJoins(bef->getNumIndexJoins() + 1);

  // now do the standard thing one should do with a substitute
  subs->allocateAndPrimeGroupAttributes();
  leftScan->getGroupAttr()->addCharacteristicInputs(ixi->inputsToIndex_);

  subs->pushdownCoveredExpr
    (subs->getGroupAttr()->getCharacteristicOutputs(),
     subs->getGroupAttr()->getCharacteristicInputs(),
     subs->selectionPred());

  // In pushdownCoveredExpr, while computing characteristics inputs
  // and outputs of the children of Join, we minimize the set of
  // coverSubExpr such that we do not produce a value and an expression
  // that depends on a value.
  //  E.g. a,b,a+b  ==> a,b
  //       a,a + 1 ==> a

  // Normally this does not causes any problem. But in case of index
  // joins, if an expression is on a clustering key, then both the
  // clustering key and the expression form the characteristics output
  // of the group (see Scan::addIndexinfo for adding clustering key)
  // Now, while minimizing the characteristics outputs, the expression
  // is removed from the outputs of the left child. The right child also does
  // not produce it, because it thinks that since it is covered by the left child
  // it should have been taken care of there. Nested join cannot produce anything
  // by itself. Hence we are left in a situation where no expression is producing
  // the required outputs. So the only resort that we have is to force the
  // right child to produce all those characteristic outputs
  // that an index join should produce, but are not being produced by the left
  //  child.If it is not added back, it will cause an assertion in the
  // Generator. Sol: 10-040227-3621

  // all these outputs are needed by the index join
  ValueIdSet reqdOutputsForParent = subs->getGroupAttr()->getCharacteristicOutputs();

  reqdOutputsForParent.subtractSet(leftScan->getGroupAttr()->getCharacteristicOutputs());
  rightScan->getGroupAttr()->addCharacteristicOutputs(reqdOutputsForParent);

  // QSTUFF
  // push stream and delete logical property to left child
  // never push anything to right child. Both scans inhert the old
  // access options which have the UpdateOrDelete flag set to
  // cause exclusive locks to be acquired.
  leftScan->getGroupAttr()->setStream(
                bef->getGroupAttr()->isStream());
  leftScan->getGroupAttr()->setSkipInitialScan(
                bef->getGroupAttr()->isSkipInitialScan());
  leftScan->getGroupAttr()->setEmbeddedIUD(
                bef->getGroupAttr()->getEmbeddedIUD());

  // synthesize logical properties for the new leaf nodes
  subs->setCursorUpdate(TRUE);
  // QSTUFF
  leftScan->synthLogProp();
  rightScan->synthLogProp();

  return subs;
}

// -----------------------------------------------------------------------
// methods for IndexJoinRule2
// -----------------------------------------------------------------------

NABoolean IndexJoinRule2::topMatch (RelExpr * expr,
                                    Context * context)
{
  // for now, return FALSE for this rule (failures in TEST005)
  return FALSE;

#pragma nowarn(203)   // warning elimination
  if (NOT Rule::topMatch(expr, context))
#pragma warn(203)  // warning elimination
    return FALSE;

  Scan * s = (Scan *) expr;

  // Disable the rule for sampleScan for now.
  if (s->isSampleScan() == TRUE)
    return FALSE;

  // go through all indexes and find out in which way they could
  // be used, if this is not already done
  s->addIndexInfo();

  // in pass 2, consider up to MAX_NUM_INDEX_JOINS index joins
  return (s->getNumIndexJoins() < Scan::MAX_NUM_INDEX_JOINS AND
          s->getIndexInfo().entries() > 0);
}

RelExpr * IndexJoinRule2::nextSubstitute(
     RelExpr * before,
     Context * /*context*/,
     RuleSubstituteMemory * & memory)
{
  return nextSubstituteForPass(before,memory,
                               RuleSet::getSecondPassNumber());
}


// -----------------------------------------------------------------------
// Methods for OrOptimizationRule
// -----------------------------------------------------------------------

NABoolean OrOptimizationRule::topMatch (RelExpr * expr,
					Context * context)
{
  if (NOT Rule::topMatch(expr,context))
    return FALSE;

  Scan *s = (Scan *) expr;
  const ValueIdSet preds = expr->getSelectionPredicates();

  // apply this rule only if there is an OR on the top of the
  // predicate tree, this must mean that we have a single entry
  if (preds.entries() != 1 OR
      NOT CURRSTMT_OPTDEFAULTS->isOrOptimizationEnabled())
    return FALSE;

  ValueId vid;

  preds.getFirst(vid);

  // the one predicate we found must be an OR
  if (vid.getItemExpr()->getOperatorType() != ITM_OR)
    return FALSE;

  // don't apply to embedded update/delete operators
  if (expr->getGroupAttr()->isEmbeddedUpdateOrDelete())
    return FALSE;

  // go through all indexes and find out in which way they could
  // be used, if this is not already done (needed for next step)
  s->addIndexInfo();

  // don't apply rule if there isn't at least one alternate index
  // (we want to make a union of at least two different indexes)
  if ((s->getIndexOnlyIndexes().entries() +
       s->getPossibleIndexJoins().entries()) < 2)
    return FALSE;

  return TRUE;
}

// helper struct for OrOptimizationRule::nextSubstitute()
// (should really be local to that method, but some C++ compilers
// don't like local structs)
struct IndexToDisjuncts
{
  friend class OrOptimizationRule;

  ValueIdSet    disjuncts_;      // disjuncts associated with this object
  CostScalar    maxPoints_;      // highest penalty points scored by a disjunct

private:
  // private constructor, object should be created by a friend only
// warning elimination (removed "inline")
  IndexToDisjuncts() : maxPoints_(0.0)
  {}
};

//10-050310-5477:
//Helper Function for OR-Optimization.
NABoolean doesValueIdEvaluateToFalse( ValueId predId )
{
    OperatorTypeEnum OpType = predId.getItemExpr()->getOperatorType();
    if( OpType == ITM_RETURN_FALSE )
    {
        return TRUE;
    }
    else if( OpType == ITM_CONSTANT )
    {
        NABoolean negate;
        ConstValue *cv = predId.getItemExpr()->castToConstValue(negate);
	    if( cv && cv->getType()->getTypeQualifier() == NA_BOOLEAN_TYPE )
		{
            Int32 val = *((Int32*)cv->getConstValue());
            if( val == 0 )
			{
			    return TRUE;
			}
		}
	 }
     return FALSE;
}

#pragma nowarn(262)   // warning elimination
RelExpr * OrOptimizationRule::nextSubstitute(
     RelExpr * before,
     Context * /*context*/,
     RuleSubstituteMemory * & /*memory*/)
{
  Scan                *s = (Scan *) before;
  MapValueIds         *result = NULL;
  ValueId             vid;

  ValueIdSet          disjuncts;
  ValueIdSet          disjunctsProcessedSoFar;
  ValueIdSet          tableColumns = s->getTableDesc()->getColumnList();
  const ValueIdList   &tableColumnList = s->getTableDesc()->getColumnList();
  const ValueIdList   &tableColumnVEGList =
                                       s->getTableDesc()->getColumnVEGList();
  CollIndex           numCols = tableColumns.entries();
  ValueIdList         charOutputList;
  ValueIdList         coPartialResult;
  RelExpr             *partialResult = NULL;

  // a sparse array that can be used to look up which index we have selected
  // for predicates on a particular column (identified by column number)
  ARRAY(CollIndex)    indexInfoByColNum(CmpCommon::statementHeap());

  // a sparse array that stores the associated disjuncts for each index,
  // arranged by index number in the scan node
  ARRAY(IndexToDisjuncts *) disjunctsByIndex(CmpCommon::statementHeap());

  CollIndex           numIndexDescs = s->numUsableIndexes();
  IndexToDisjuncts    *idinfo = NULL;

  charOutputList.insertSet(s->getGroupAttr()->getCharacteristicOutputs());
  tableColumns.insertList(tableColumnVEGList);

  // ---------------------------------------------------------------------
  // Part 1: Determine which indexes to use and how many UNION nodes to
  //         make
  //
  // Split the predicate into disjuncts (components of an OR-backbone).
  //
  // For each disjunct, try to find a column of the table where that
  // disjunct could be used as a key predicate. Do this with a simple
  // algorithm that finds only obvious cases of key predicates such as
  // <col> = expr. Note that this may miss some other possible key
  // predicates as well as select some predicates that aren't really
  // key predicates. This is not great but acceptable, since we are
  // dealing with a conditional transformation in the optimizer. Give
  // up if any disjunct does not satisfy this condition, since it
  // would cause a full table scan, defeating the purpose of
  // OR-optimization.
  //
  // Now "go shopping for an index" for each unique column obtained in
  // step 3. Walk through the list of index-only indexes and index
  // join indexes. If the index contains our column, compute the
  // approximate number of probes needed.
  //
  // a) To calculate the number of probes, start with the UEC count of
  //    all columns that precede our column number in the index. Use 1 if
  //    the column is the leading column in the index. This approximates
  //    the number of MDAM probes.
  //
  // b) To this, add the number of rows returned by the scan node if
  //    this is an index join. This is a very crude approximation of the
  //    number of probes in the index join. Do not add any probes if the
  //    index alone covers all the characteristic outputs of the
  //    group. In this case we heuristically assume that local predicates
  //    don't require any base table columns, either. Note that we do the
  //    index selection only once per column and not once per disjunct,
  //    this is why we want to avoid looking at the predicate here.
  //
  // c) If an index has already been selected by a previous disjunct,
  //    count only the additional probes incurred by this disjunct, if
  //    any.
  //
  // d) Select the index with the lowest number of probes (using a
  //    simple tie breaker if necessary).
  //
  //
  // get all disjunctive parts of the OR predicate (note that topMatch()
  // made sure there is only one entry in the selection predicate ValueIdSet)
  // ---------------------------------------------------------------------

  ValueIdSet selPreds = s->getSelectionPredicates();
  if ((CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON ) &&
      (selPreds.entries()))
  {
    ItemExpr * inputItemExprTree = selPreds.rebuildExprTree(ITM_AND,FALSE,FALSE);
    ItemExpr * resultOld = revertBackToOldTree(CmpCommon::statementHeap(), inputItemExprTree);
    ValueIdSet convpredicates;
    resultOld->convertToValueIdSet(convpredicates,  NULL, ITM_AND);
    doNotReplaceAnItemExpressionForLikePredicates(resultOld,convpredicates,resultOld);
    convpredicates.getFirst(vid);
//     ValueIdSet resultSet;
//	 revertBackToOldTreeUsingValueIdSet(selPreds, resultSet);
//	 ItemExpr * resultOld =  resultSet.rebuildExprTree(ITM_AND,FALSE,FALSE);
//	 doNotReplaceAnItemExpressionForLikePredicates(resultOld,resultSet,resultOld);
//     resultSet.getFirst(vid);
  }
  else
  {
    s->getSelectionPredicates().getFirst(vid);
  }
  vid.getItemExpr()->convertToValueIdSet(disjuncts, NULL, ITM_OR, FALSE);

  // ---------------------------------------------------------------------
  // Go through the disjuncts, find a column for which they can form
  // a key predicate (give up if none found), and find an index to use.
  // As mentioned above, this code recognizes only simple key predicates.
  // ---------------------------------------------------------------------
  // 10-050310-5477: If we have disjuncts of the form 1=2 or i = 30 or j = 40
  // or k = 50; We used to ignore the valid disjuncts due to the idempotent
  // condition 1=2. So the code change will scan all the disjuncts.
  // ---------------------------------------------------------------------
  ValueIdSet  disjunctsEvaluatingToFalse;
  for (ValueId d=disjuncts.init(); disjuncts.next(d); disjuncts.advance(d))
    {
      ItemExpr *ie = d.getItemExpr();
      ItemExpr *col = NULL;

      switch (ie->getOperatorType())
	{
	case ITM_VEG_PREDICATE:
	  {
	    // we should not really see VEGPredicates in a simple OR, but
	    // who knows, the monkey on the keyboard will generate this...
	    VEGPredicate *v = (VEGPredicate *) ie;
	    VEGReference *r = v->getVEG()->getVEGReference();

	    if (tableColumns.contains(r->getValueId()))
			col = r;

	  }
	  break;

	case ITM_EQUAL:
	case ITM_LESS:
	case ITM_LESS_EQ:
	case ITM_GREATER:
	case ITM_GREATER_EQ:
	  {
	    BiRelat *br = (BiRelat *) ie;
	    ItemExpr *leftOp = br->child(0);
	    ItemExpr *rightOp = br->child(1);
	    if (tableColumns.contains(leftOp->getValueId()))
	      col = leftOp;
	    else if (tableColumns.contains(rightOp->getValueId()))
	      col = rightOp;
		else
		{
            //10-050310-5477: We have a case where we have
			// both the LHS and RHS are not columns. We can run
			// into this case if we have a FALSE or TRUE or ? = ?
			// as one of the disjunct.
			// 1] For TRUE and ?=? we have do a full table scan anyways,
			// so break and Return NULL as the nextSubstitute
			// 2] For the FALSE case we need to continue and with the
			// other disjuncts. As we may still use OR-optimization.
			// But mark this valueID we will remove it from our set
			// of disjucts we are processing.

			// If we are here we have disjunct of the form ? = ?
			// No columns and falls into case 1]
			return NULL;
		}
	  }
	  break;
	case ITM_CONSTANT:
	  // We have systemliteral FALSE or TRUE. case 2]

          // If a predicate contains 1=2, it is constant folded and the entire
          // OR predicate is TRUE. We never hit this rule;
          // If a predicate contains 1=1, it is constant folded and removed
          // from the OR predicate.
          //
          // LCOV_EXCL_START
      if( doesValueIdEvaluateToFalse(d) )
	  {
	      disjunctsEvaluatingToFalse += d;
	  }
  	  else
	  {
	      return NULL;
	  }
	  break;
          // LCOV_EXCL_STOP

	default:
	  // leave col set to NULL
	  // Return from here to maintain semantics.
	  return NULL;
	}
    // 10-050310-5477
    // if we can't associate the disjunct with any column then there is
    // no point in doing OR-optimization, because we would have to do a
    // full table scan for this particular disjunct anyway. But we need
	// to go throught all the disjuncts.
    if(NOT col)
       continue;

	DCMPASSERT(col);
      // Calculate the column number that this particular disjunct is
      // using in a comparison.
      CollIndex colNum = numCols; // initialize with invalid number
      ValueId colValId = col->getValueId();

      // calculate the column number
      for (CollIndex c=0; c < numCols; c++)
	if (colValId == tableColumnList[c] OR
	    colValId == tableColumnVEGList[c])
	  {
	    colNum = c;
	    break;
	  }

      DCMPASSERT(colNum < numCols);

      // -----------------------------------------------------------------
      // Don't compute the index to use more than once for a given column
      // number, since this algorithm is independent of the predicate
      // (at least right now)
      // -----------------------------------------------------------------
      CostScalar bestIxPoints   = 0.0;
      CostScalar ixPoints;

      if (indexInfoByColNum.used(colNum))
	{
	  // re-use the column to index mapping established earlier
	  idinfo = disjunctsByIndex[indexInfoByColNum[colNum]];
	}
      else
	{
	  // -------------------------------------------------------------
	  // Now "go shopping" for indexes. Walk through all indexes
	  // (both index-only and index joins) and check whether the
	  // index contains column <colNum>. If it does, compute a
	  // measure on how good the index is and pick the index that
	  // is best according to the measure.  Keep an array that
	  // associates a set of disjuncts, the worst measure of these
	  // disjuncts, and a set of alternative indexes and
	  // index-joins with each index.
	  // -------------------------------------------------------------

	  CollIndex  bestIxNum      = NULL_COLL_INDEX;
	  IndexDesc *ixDesc;
	  Int32 colNumInIndex;
	  CollIndex ixNum           = 0; // artificial numbering scheme

	  CollIndex numIndexDescs = s->numUsableIndexes();
	  IndexProperty **indexOnlyInfo = NULL;
	  ScanIndexInfo **indexJoinInfo = NULL;

	  // walk over the indexes for this scan node
	  for (ixNum = 0; ixNum < numIndexDescs; ixNum++)
	    {
	      ixDesc = s->getUsableIndex(ixNum, indexOnlyInfo);

	      // does the index contain column <colNum>?
	      colNumInIndex = -1;

	      for (CollIndex ixcolnum=0;
		   ixcolnum < ixDesc->getIndexKey().entries();
		   ixcolnum++)
		{
		  IndexColumn *ic =	(IndexColumn *)
		    ixDesc->getIndexKey()[ixcolnum].getItemExpr();
		  BaseColumn *bc =
		    (BaseColumn *) ic->getDefinition().getItemExpr();

		  DCMPASSERT(bc->getOperatorType() == ITM_BASECOLUMN);
		  if (colNum == (CollIndex) bc->getColNumber())
		    {
#pragma nowarn(1506)   // warning elimination
		      colNumInIndex = ixcolnum;
#pragma warn(1506)  // warning elimination
		      break;
		    }
		}

	      // can this disjunct (probably) be used as a key predicate for
	      // this index?
	      if (colNumInIndex >= 0)
		{
		  ixPoints = rateIndexForColumn(colNumInIndex,
						s,
						ixDesc,
						(indexOnlyInfo != NULL));

		  // subtract shared penalty points with other disjuncts
		  // (see comments above why this is done)
		  if (disjunctsByIndex.used(ixNum))
		    {
		      CostScalar prevPoints =
			disjunctsByIndex[ixNum]->maxPoints_ ;
		      if (prevPoints < ixPoints)
			ixPoints -= prevPoints;
		      else
			ixPoints = 0.0;
		    }

		  // compare to best index so far, if any
		  if (bestIxNum == NULL_COLL_INDEX OR ixPoints < bestIxPoints)
		    {
		      bestIxPoints = ixPoints;
		      bestIxNum    = ixNum;
		    }
		}
	    } // end of loop that walks through index descs

	  // Did we decide on an index to use for this disjunct?  Give
	  // up if we didn't (e. g. there was no index on the column
	  // in the predicate)
	  if (bestIxNum == NULL_COLL_INDEX)
	    return NULL;

	  // remember the best index for column # <colNum> so we don't
	  // have to go through this loop again for the same column
	  indexInfoByColNum.insertAt(colNum, bestIxNum);

	  // record information about the association between this index and
	  // the disjunct in the array <disjunctsByIndex>
	  if (NOT disjunctsByIndex.used(bestIxNum))
	    {
	      disjunctsByIndex.insertAt(
		   bestIxNum,
		   new (CmpCommon::statementHeap()) IndexToDisjuncts);
	      idinfo = disjunctsByIndex[bestIxNum];
	    }
	  else
	    idinfo = disjunctsByIndex[bestIxNum];
	} // did not compute score for this column number yet

      // at this point we have calculated the pointer idinfo that
      // points to the best index (heuristically chosen) for this
      // disjunct
      idinfo->disjuncts_ += d;
      if (idinfo->maxPoints_ < bestIxPoints)
	idinfo->maxPoints_ = bestIxPoints;

    } // end of for loop over disjuncts

	// 10-050310-5477
	// if we can't associate the disjunct with any column then there is
    // no point in doing OR-optimization, because we would have to do a
    // full table scan for this particular disjunct anyway. Ideally here
	// we have not disjuncts with column names so the expression should
	// be folded to the maximum possible extent.
    if(disjunctsEvaluatingToFalse == disjuncts)
	   return NULL;

	//  10-050310-5477
	//  Remove all those idempotent disjuncts.
	if(disjunctsEvaluatingToFalse.entries())
		disjuncts -= disjunctsEvaluatingToFalse;


	// 10-050310-5477
	// if we can't associate the disjunct with any column then there is
    // no point in doing OR-optimization, because we would have to do a
    // full table scan for this particular disjunct anyway. Ideally here
	// we have not disjuncts with column names so the expression should
	// be folded to the maximum possible extent.
    if(disjunctsEvaluatingToFalse == disjuncts)
	   return NULL;

	//  10-050310-5477
	//  Remove all those idempotent disjuncts.
	if(disjunctsEvaluatingToFalse.entries())
		disjuncts -= disjunctsEvaluatingToFalse;


  // if we haven't found more than one usable index then all this work
  // was for nothing, since we can't produce two different scan nodes
  if (disjunctsByIndex.entries() < 2)
    return NULL;

  // ---------------------------------------------------------------------
  // Part 2: Now create the actual substitute
  // ---------------------------------------------------------------------

  // now walk over the stored indexes and disjuncts and create a new scan
  // node for each entry in the disjunctsByIndex array
  for (CollIndex currIndexNum=0; currIndexNum < numIndexDescs; currIndexNum++)
    {
      if (disjunctsByIndex.used(currIndexNum))
	{
	  IndexToDisjuncts &idinfo = *(disjunctsByIndex[currIndexNum]);

	  // create a new scan node or a scan+union node
	  partialResult = makeSubstituteScan(
	       s,
	       idinfo.disjuncts_,
	       partialResult,
	       disjunctsProcessedSoFar,
	       charOutputList,
	       coPartialResult);

	  // remember the disjuncts used so far, to add their negation
	  // to future scan nodes
          disjunctsProcessedSoFar += idinfo.disjuncts_;
	} // entry of disjunctsByIndex is used
    } // end of walk over disjunctsByIndex array

  DCMPASSERT(disjunctsProcessedSoFar == disjuncts);

  // ---------------------------------------------------------------------
  // Create a MapValueIds node that maps the ValueIdUnion nodes to the
  // original characteristics outputs, this is the result node of this rule.
  // ---------------------------------------------------------------------
  result = new (CmpCommon::statementHeap())
    MapValueIds(partialResult, ValueIdMap(charOutputList, coPartialResult));
  result->setGroupAttr(s->getGroupAttr());

  // To be able to replace VEGies later for the upper values, remember
  // that this map value ids really represents a scan node and that we should
  // choose one of the columns of the scan node from any VEGies that need
  // to be resolved here and have such a column as a VEG member.
  result->addValuesForVEGRewrite(tableColumnList);

  // now do the standard thing one should do with a substitute
  result->allocateAndPrimeGroupAttributes();
  result->pushdownCoveredExpr
    (result->getGroupAttr()->getCharacteristicOutputs(),
     result->getGroupAttr()->getCharacteristicInputs(),
     result->selectionPred());

  // synthesize logical properties for the new nodes below the result
  result->child(0)->synthLogProp();

  // We don't want the union nodes to have a "NumBaseTables" attribute
  // that is greater than one, because the group of the result will
  // usually have its NumBaseTables attribute set to 1. Go and adjust
  // this property in the newly generated union nodes.
  Union *u = (Union *) result->child(0).getPtr();
  while (u->getOperatorType() == REL_UNION)
    {
      u->getGroupAttr()->resetNumBaseTables(1);
      u = (Union *) u->child(0).getPtr();
    }

  return result;
}
#pragma warn(262)  // warning elimination

CostScalar OrOptimizationRule::rateIndexForColumn(
     Int32 colNumInIndex,
     Scan *s,
     IndexDesc *ixDesc,
     NABoolean indexOnly)
{
  // yes, try to estimate how useful the index would be:
  // - Add one penalty point for each "MDAM skip" we would
  //   do, approximated by the combined UEC of the columns
  //   in the index before <colNumInIndex>.
  // - Add one penalty point for each probe we would do into
  //   the inner table of an index join, approximate this by
  //   the estimated rowcount (ignoring the fact that this
  //   disjunct probably selects fewer rows)
  // - don't count those penalty points that have already
  //   been charged to a previous disjunct, since the current
  //   and previous disjunct will share a common scan node
  CostScalar result = 1.0;

  // collect penalty points for non-leading index column
  if (colNumInIndex > 0)
    {
      // $$$$ replace with real UEC later
      result += colNumInIndex;
    }

  // collect penalty points for index joins
  if (NOT indexOnly)
    {
      // Add a penalty for doing the index join, BUT do this
      // only if the newly created scan node for this
      // disjunct really needs an index join. It could be
      // that the base table was needed only to evaluate
      // predicates. The logic below tries to recognize this
      // case. We make the optimistic assumption that our
      // local predicate does NOT require values from the
      // base table (this is because we don't want to go
      // through this calculation for each disjunct, we just
      // want to do it once per column number). Note that
      // if this is the wrong decision we still pick an
      // index that could be used, it is just more expensive
      // than it would otherwise be.
      GroupAttributes indexOnlyGA;
      ValueIdSet dummyReferencedInputs;
      ValueIdSet dummyCoveredSubExpr;
      ValueIdSet dummyUnCoveredExpr;
      indexOnlyGA.addCharacteristicInputs(
	   s->getGroupAttr()->getCharacteristicInputs());
      indexOnlyGA.addCharacteristicOutputs(
	   ixDesc->getIndexColumns());
      if (s->getGroupAttr()->
	  getCharacteristicOutputs().isCovered(
	       s->getGroupAttr()->getCharacteristicInputs(),
	       indexOnlyGA,
	       dummyReferencedInputs,
	       dummyCoveredSubExpr,
	       dummyUnCoveredExpr))
	{
	  // hope that we won't have to do an index join;
	  // add a half penalty point for the risk that we
	  // may be wrong, this makes sure that an otherwise
	  // equal index-only index will win
	  result += 0.5;
	}
      else
	{
	  // expect to do an index join, assume that we have
	  // to probe into base table once for every row
	  // that all of the predicates produce
	  result += s->getGroupAttr()->getResultCardinalityForEmptyInput();
	}
    }

  return result;
}

RelExpr * OrOptimizationRule::makeSubstituteScan(
     Scan *s,
     const ValueIdSet &disjuncts,
     RelExpr *partialResult,
     const ValueIdSet disjunctsProcessedSoFar,
     const ValueIdList &origCharOutputList,
     ValueIdList &resultCharOutputs)
{
  RelExpr      *result = NULL;
  BindWA       bindWA(ActiveSchemaDB(), CmpCommon::context());
  NormWA       normWA(CmpCommon::context());
  CorrName     cn(s->getTableName().getQualifiedNameObj(),
		  CmpCommon::statementHeap());

  // -------------------------------------------------------------
  // Make a new scan node
  // -------------------------------------------------------------
  Scan *newScan = new (CmpCommon::statementHeap()) Scan(
       cn,
       bindWA.createTableDesc(s->getTableDesc()->getNATable(),
			      cn,
			      FALSE));
  newScan->setOptStoi(s->getOptStoi());
  newScan->accessOptions()  = s->accessOptions();

  // QSTUFF
  // propagate stream properties to new scan node
  newScan->getGroupAttr()->setStream(s->getGroupAttr()->isStream());
  newScan->getGroupAttr()->setSkipInitialScan(
       s->getGroupAttr()->isSkipInitialScan());
  // QSTUFF

  // don't apply the OR-optimization rule on the results again, it
  // shouldn't find any good conditions for further OR-optimization
  newScan->contextInsensRules() += getNumber();

  // Go through normalization of the new scan node, VEGies will be
  // created for each column during this step. Unlike for joins,
  // creating new UNION nodes requires new VEG regions: The
  // values in the different unions aren't the same and shouldn't
  // be members of a VEG.
  ExprGroupId dummyPtr = newScan;
  normWA.allocateAndSetVEGRegion(IMPORT_ONLY,newScan,0);
  newScan->transformNode(normWA, dummyPtr);
  newScan->rewriteNode(normWA);
  newScan->normalizeNode(normWA);

  const ValueIdList &vegCols =
    s->getTableDesc()->getColumnVEGList();
  const ValueIdList &newVegCols =
    newScan->getTableDesc()->getColumnVEGList();

  ValueIdMap newUnionMap(s->getTableDesc()->getColumnList(),
			 newScan->getTableDesc()->getColumnList());
  for (CollIndex v=0; v < vegCols.entries(); v++)
    newUnionMap.addMapEntry(vegCols[v],newVegCols[v]);

  // assign the disjuncts to the new scan nodes
  ValueIdSet vs;
  ItemExpr *newDisjuncts = NULL;

  // take the disjunct(s) destined for the new scan node and
  // rewrite them in terms of the value ids of the new scan
  // node, then add them to the new scan's selection
  // predicates
  newUnionMap.rewriteValueIdSetDown(disjuncts, vs);
  newDisjuncts = vs.rebuildExprTree(ITM_OR);
  newDisjuncts->synthTypeAndValueId();
  newScan->selectionPred() += newDisjuncts->getValueId();

  // compute the characteristic outputs of the new scan node,
  // otherwise the standard method of calling
  // allocateAndPrimeGroupAttributes and pushdownCoveredExpr
  // below will use too many outputs
  vs.clear();
  newUnionMap.rewriteValueIdSetDown(
       s->getGroupAttr()->getCharacteristicOutputs(), vs);
  newScan->setPotentialOutputValues(vs);

  if (partialResult)
    {
      // We'll now take the disjuncts that were processed so
      // far and AND their negation to the selection predicate
      // of the new scan node. Without this, we would return
      // those rows twice that satisfy both the previous and
      // the current disjuncts. Note that "negation" in this
      // case means "NOT ((<pred>) IS TRUE)", we only want to
      // exclude those rows where the left part of the OR
      // evaluated to TRUE.

      ItemExpr * negatedExprForNewScan;
      Union    * unionNode;

      vs.clear();
      newUnionMap.rewriteValueIdSetDown(disjunctsProcessedSoFar, vs);
      negatedExprForNewScan = vs.rebuildExprTree(ITM_OR);
      negatedExprForNewScan =
	new (CmpCommon::statementHeap()) UnLogic(
	     ITM_NOT,
	     new (CmpCommon::statementHeap()) UnLogic(
		  ITM_IS_TRUE,
		  negatedExprForNewScan));
      negatedExprForNewScan->synthTypeAndValueId();
      newScan->selectionPred() += negatedExprForNewScan->getValueId();

      // ---------------------------------------------------------
      // create the UNION node that connects the two new scans
      // ---------------------------------------------------------
      result =
      unionNode =
	new(CmpCommon::statementHeap()) Union(partialResult, newScan);

      ValueIdList newPartialResult;

      // from Union::bindNode(): Make the map of value ids for
      // the union
      for (CollIndex i = 0; i < origCharOutputList.entries(); i++)
	{
	  ValueId newValId;

	  // translate the value id of the characteristic
	  // outputs into the equivalent value id of the
	  // children
	  newUnionMap.rewriteValueIdDown(origCharOutputList[i],newValId);

	  ValueIdUnion *vidUnion = new (CmpCommon::statementHeap())
	    ValueIdUnion(resultCharOutputs[i],
			 newValId,
			 NULL_VALUE_ID,
#pragma nowarn(1506)   // warning elimination
			 unionNode->getUnionFlags());
#pragma warn(1506)  // warning elimination
	  vidUnion->synthTypeAndValueId();
	  unionNode->addValueIdUnion(vidUnion->getValueId(),
				     CmpCommon::statementHeap());
	  newPartialResult.insert(vidUnion->getValueId());
	}

      // store the value ids of the newly created partial result
      resultCharOutputs = newPartialResult;
    }
  else
    {
      // pass the newly created scan node and the union map
      // (via prevUnionMap) on to the next iteration
      result = newScan;
      newUnionMap.rewriteValueIdListDown(origCharOutputList,
					 resultCharOutputs);
    }
  return result;
}

// -----------------------------------------------------------------------
// methods for class RoutineJoinToTSJRule
// -----------------------------------------------------------------------

RoutineJoinToTSJRule::~RoutineJoinToTSJRule() {}

NABoolean RoutineJoinToTSJRule::topMatch(RelExpr *relExpr,
                                      Context *context)
{
  // Fasttrack out of here if this is something other than a RoutineJoin.
  if (relExpr->getOperatorType() != REL_ROUTINE_JOIN)
    return FALSE;

  if (NOT Rule::topMatch(relExpr, context))
    return FALSE;

  Join *joinExpr = (Join * )relExpr;

  // nested join is not good for filtering joins in the star join schema
  // This rule is for when we merge with JoinToTSJ rule
  if ((CmpCommon::getDefault(COMP_BOOL_85) == DF_OFF &&
      joinExpr->getSource() == Join::STAR_FILTER_JOIN) &&
      ! joinExpr->isRoutineJoin())
    return FALSE;

  return TRUE;
} // RoutineJoinToTSJRule::topMatch

RelExpr *RoutineJoinToTSJRule::nextSubstitute(
                             RelExpr *before,
                             Context *context,
                             RuleSubstituteMemory * & memory)
{
  Join *bef = (Join * )before;

  CMPASSERT(bef->isRoutineJoin());
  
  // Want to make sure we don't have a join predicate..
  CMPASSERT(bef->joinPred().isEmpty());


  // Build the result tree,
  // Make sure we call the copyTopNode for the Join.
  Join *result = (Join *)Rule::nextSubstitute(before,NULL,memory);

  result->setOperatorType(REL_TSJ);

  // now set the group attributes of the result's top node
  result->setGroupAttr(before->getGroupAttr());

  // transfer all the join fields to the new TSJ
  (void) bef->Join::copyTopNode(result,CmpCommon::statementHeap());


  // Recompute the input values that each child requires as well as
  // the output values that each child is capable of producing
  result->allocateAndPrimeGroupAttributes();

  ValueIdSet availableOutputsFromLeftChild = 
                before->child(0).getGroupAttr()->getCharacteristicOutputs();

  ValueIdSet availableInputs = availableOutputsFromLeftChild;
             availableInputs += result->getGroupAttr()->getCharacteristicInputs();

  ValueIdSet exprOnParent;

   // Push the predicate to the righ child. We already have pushed 
   // it to the left in Join::pushdownCoveredExpr()
  result->RelExpr::pushdownCoveredExpr(
               result->getGroupAttr()->getCharacteristicOutputs(),
               availableInputs,
               result->selectionPred(), &exprOnParent, 1);


  // We better have pushed everything..
  CMPASSERT(result->selectionPred().isEmpty());

  RelExpr * filter = result->child(1);

  // Eliminate Filter node if no predicates were added to it
  if (filter->selectionPred().isEmpty())
    {
      result->child(1) = filter->child(0);
      filter->deleteInstance();
      filter = NULL;
    }


  // synthesize logical properties for my new right child
  if (filter)
    {
      filter->synthLogProp();
      filter->getGroupAttr()->addToAvailableBtreeIndexes(
        filter->child(0).getGroupAttr()->getAvailableBtreeIndexes());

      if (before->getInliningInfo().isMaterializeNodeForbidden())
        filter->getInliningInfo().setFlags(II_MaterializeNodeForbidden);
    }

  result->setDerivedFromRoutineJoin();
  
  return result;

} // RoutineJoinToTSJRule::nextSubstitute()

Int32 RoutineJoinToTSJRule::promiseForOptimization(RelExpr * ,
                                            Guidance * ,
                                            Context * )
{
  // Transforming logical joins to logical TSJs enable the physical
  // NestedJoin rule to fire.  Thus, we would like this rule to fire
  // prior to other join transformation rules.
  return (Int32)(DefaultTransRulePromise - 1000 /*+ 1000*/);
}

NABoolean RoutineJoinToTSJRule::canBePruned(RelExpr * expr) const
{
  // We do not want to prune RJ->TSJ if expr is an RoutineJoin
  // since thats its only possible implementation in many cases
  return NOT expr->getOperator().match(REL_ROUTINE_JOIN);
}

// -----------------------------------------------------------------------
// methods for JoinToTSJRule
// -----------------------------------------------------------------------
NABoolean JoinToTSJRule::topMatch (RelExpr * expr,
                                   Context * context)
{
  if (NOT Rule::topMatch(expr, context))
    return FALSE;

  Join *joinExpr = (Join *) expr;

  if (joinExpr->isRoutineJoin()) 
    return FALSE; // don't want to to this for a routine join 

/*
   The block was added when JoinToTSJRule was a pass 2 rule by default
   This is changed now and it is a pass 1 plan so this code should be
   removed. I gurded it with comp_bool_71 however and not removed it in case
   we want to set it to be pass2 rule again, then we will have to enable
   this code.
*/

  // The JoinToTSJRule is enabled in Pass1 ONLY if there is at least
  // one embedded update or delete or a stream in the query. For pass1
  // fire the rule only for embedded updates and deletes and for streams.
  if ((CmpCommon::getDefault(COMP_BOOL_71) == DF_ON) AND
      (GlobalRuleSet->getCurrentPassNumber() ==
         GlobalRuleSet->getFirstPassNumber()) AND
       NOT (joinExpr->child(0).getGroupAttr()->isEmbeddedUpdateOrDelete() OR
	    joinExpr->child(0).getGroupAttr()->isStream())
     )
    return FALSE;


  // QSTUFF
  CMPASSERT (NOT ((Join *)expr)->child(1).getGroupAttr()->isStream());
  CMPASSERT (NOT ((Join *)expr)->child(1).getGroupAttr()->isEmbeddedUpdateOrDelete());
  // QSTUFF

  RelExpr * rightChild = joinExpr->child(1);
  if (rightChild && rightChild->getGroupAttr()->getNumTMUDFs() > 0)
    return FALSE;

  // nested join excluded by heuristics
  //if (NOT joinExpr->canTransformToTSJ()) return FALSE;

  // QSTUFF
  // streams, embedded deletes and embedded updates
  // must use tsj's and can't use merge or hash joins.
  if (joinExpr->getGroupAttr()->isStream() ||
      joinExpr->getGroupAttr()->isEmbeddedUpdateOrDelete() )
    return TRUE;
  // QSTUFF

  // avoid keyless NJs: nested joins that require full table scans.
  // skip keyless NJ if all of the following are true:
  //   1) no cqs
  //   2) nested join is not the only explored implementation option
  //   3) cqd to skip keyless NJ is ON
  //   4) we have query analysis

  // if there is a required shape, skip this keylessNJ heuristic.
  NABoolean ignoreCQS = 
    ((ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_71) == 1);
  NABoolean cqs_skips_keylessNJ_heuristic = 
    (ignoreCQS ? FALSE :
     (context && context->getReqdPhysicalProperty()->getMustMatch() != NULL));
    
  NABoolean canUseMJ = CURRSTMT_OPTDEFAULTS->isMergeJoinConsidered() && 
                       !(joinExpr->getEquiJoinPredicates().isEmpty());
  // if NJ is the only (efficient) implementation option, skip this heuristic.
  NABoolean NJisOnlyOption =
    // NJ is the only implementation for pushdown compound stmt
    ((expr->isinBlockStmt() && CURRSTMT_OPTDEFAULTS->pushDownDP2Requested()) 
     OR
     // NJ into fact table is the only explored implementation option
     (joinExpr->getSource() == Join::STAR_FACT) 
     OR
     // NJ is the only join implementation option
     (!joinExpr->allowHashJoin() 
      OR (!CURRSTMT_OPTDEFAULTS->isHashJoinConsidered() &&
          !canUseMJ)));

  // cqd to skip keyless NJ
  NABoolean keylessNJ_off = 
    CmpCommon::getDefault(KEYLESS_NESTED_JOINS) == DF_OFF;

  // cqd to allow only full inner key NJ
  NABoolean fullInnerKey =
    CmpCommon::getDefault(NESTED_JOINS_FULL_INNER_KEY) == DF_ON;

  // do we have analysis?
  NABoolean hasAnalysis = QueryAnalysis::Instance()->isAnalysisON();

  Lng32 mtd_mdam_uec_threshold = (Lng32)(ActiveSchemaDB()->getDefaults()).
                                     getAsLong(MTD_MDAM_NJ_UEC_THRESHOLD);

  // if no cqs and NJ is not the only explored option and keylessNJ off
  // then avoid keyless nested join
  if (!cqs_skips_keylessNJ_heuristic AND !NJisOnlyOption AND 
      keylessNJ_off AND hasAnalysis)
    {
      // if there is at least 1 index
      const SET(IndexDesc *) &availIndexes=
        joinExpr->child(1).getGroupAttr()->getAvailableBtreeIndexes();
      if (availIndexes.entries() > 0)
        {
          // get all predicates
          ValueIdSet allJoinPreds;
          allJoinPreds += joinExpr->getSelectionPred();
          allJoinPreds += joinExpr->getJoinPred();

          // get all predicates' base columns
          ValueIdSet allReferencedBaseCols;
          allJoinPreds.findAllReferencedBaseCols(allReferencedBaseCols);

          NABoolean handleDivColumnsInMTD =
              (CmpCommon::getDefault(MTD_GENERATE_CC_PREDS) == DF_ON);

          if ( mtd_mdam_uec_threshold < 0 )
            handleDivColumnsInMTD = FALSE;

          NABoolean collectLeadingDivColumns = handleDivColumnsInMTD;

          NABoolean nj_check_leading_key_skew =
            (CmpCommon::getDefault(NESTED_JOINS_CHECK_LEADING_KEY_SKEW) == DF_ON);

          Lng32 nj_leading_key_skew_threshold =
             (Lng32)(ActiveSchemaDB()->getDefaults()).
                            getAsLong(NESTED_JOINS_LEADING_KEY_SKEW_THRESHOLD);

          // try to find a predicate that matches 
          // 1st nonconstant key prefix column
          NABoolean foundPrefixKey = FALSE;
          CollIndex x, i = 0;
          for (i = 0; i < availIndexes.entries() && !foundPrefixKey; i++)
            {
              IndexDesc *currentIndexDesc = availIndexes[i];
              const ValueIdList *currentIndexSortKey = 
                (&(currentIndexDesc->getOrderOfKeyValues()));

             NABoolean missedSuffixKey = FALSE;
             ValueIdSet divColumnsWithoutKeyPreds;
             ValueIdSet leadingKeys;
 
              // get this index's 1st nonconstant key prefix column
              for (x = 0; 
                   x < (*currentIndexSortKey).entries() &&
                   (!foundPrefixKey || fullInnerKey); 
                   x++)
                {
                  ValueId firstkey = (*currentIndexSortKey)[x];

                  // firstkey with a constant predicate does not count in
                  // making this NJ better than a HJ. keep going.
                  ItemExpr *cv; 
                  NABoolean isaConstant = FALSE;
                  ValueId firstkeyCol;
                  ColAnalysis *colA = firstkey.baseColAnalysis
                    (&isaConstant, firstkeyCol);
                  leadingKeys.insert(firstkeyCol);

		  if(colA)
		  {
		    if (colA->getConstValue(cv,FALSE/*useRefAConstExpr*/))
		      continue; // try next prefix column
		  }
		  else
		  {
		    ValueIdSet predsWithConst;
		    ValueIdSet localPreds = currentIndexDesc->getPrimaryTableDesc()->getLocalPreds();
		    localPreds.getConstantExprs(predsWithConst);

		    firstkeyCol = firstkey.getBaseColumn(&isaConstant);

		    ValueId exprId;
		    if(predsWithConst.referencesTheGivenValue(firstkeyCol,exprId))
		      continue;
		  }

                  // skip salted columns and constant predicates
                  if (isaConstant || firstkeyCol.isSaltColumn() ) 
                    continue; // try next prefix column

                  // If firstkeyCol is one of the leading DIVISION columns
                  // without any predicate attached on it, collect it in
                  // a ValueIdSet. Later on, we will check the UEC for the
                  // set. If the UEC is less than a threshold, we will allow
                  // such "keyless" NJ.
                  NABoolean isLeadingDivColumn =
                     firstkeyCol.isDivisioningColumn();

                  // any predicate on first nonconstant prefix key column?
                  if (allReferencedBaseCols.containsTheGivenValue(firstkeyCol))
                    {
                      if ( collectLeadingDivColumns )
                         // We will stop collect any additional leading DIV
                         // key columns from this point on.
                         collectLeadingDivColumns = FALSE;

                      // nonconstant prefix key matches predicate
                      foundPrefixKey = TRUE; // allow this NJ to compete
                    }
                  else // no predicate match for prefix key column
                    {
                      if ( collectLeadingDivColumns && isLeadingDivColumn )
                        divColumnsWithoutKeyPreds.insert(firstkeyCol);
                      else
                      {
                        missedSuffixKey = TRUE;
                        leadingKeys.remove(firstkeyCol);
                        break; // try next index
                      }
                    }
                }
              if ( handleDivColumnsInMTD && foundPrefixKey ||
                   (missedSuffixKey && nj_check_leading_key_skew))
              {
                 // If the SC/MC UEC of all leading div columns absent of
                 // key predicates is greater than a threshold specified via
                 // CQD MTD_MDAM_NJ_UEC_THRESHOLD, disable "keyless" NJ.

                 // First figure out the SC/MC UEC on the leading DIV columns
                 // lacking any key predicates, or RC/uec when the trailing
                 // key columns lacking any key predicates.

                 EstLogPropSharedPtr inputLPForChild;
                 inputLPForChild = joinExpr->child(0).outputLogProp
                              ((*GLOBAL_EMPTY_INPUT_LOGPROP));

                 EstLogPropSharedPtr outputLogPropPtr =
                    joinExpr->child(1).getGroupAttr()->
                        outputLogProp(inputLPForChild);

                 const ColStatDescList & stats = outputLogPropPtr->getColStats();
                 const MultiColumnUecList * uecList =  stats.getUecList();

                 if ( uecList ) {

                    CostScalar uec;
                    if ( missedSuffixKey && nj_check_leading_key_skew ) {

                      // max rows from the inner
                     CostScalar child1card = joinExpr->child(1).getGroupAttr()
                                   ->getResultCardinalityForEmptyInput();

                      uec = uecList->lookup(leadingKeys);

                      if ( uec.isGreaterThanZero() &&
                        child1card/uec <=
                            CostScalar(nj_leading_key_skew_threshold) )
                         missedSuffixKey = FALSE;

                    } else {
                      uec = uecList->lookup(divColumnsWithoutKeyPreds);
                      if ( uec.isGreaterThanZero() &&
                           uec > CostScalar(mtd_mdam_uec_threshold) )
                        foundPrefixKey = FALSE;
                    }
                 }
              }
              // skip partial keyless NJ if all key cols are not covered
              if (foundPrefixKey && missedSuffixKey)
                foundPrefixKey = FALSE; 
       
            }
          // all indexes have been tried
        
          if (!foundPrefixKey)
            return FALSE; // it's a keyless NJ. avoid it.
        }
      else // no index
        {
          if ( CmpCommon::getDefault(NESTED_JOINS_KEYLESS_INNERJOINS) == DF_OFF )
            return FALSE; // it's a keyless NJ. avoid it.
          else {
          // When the CQD is ON, we rely on the code above for the contained NJs 
          // in the inner, the max cardinality and risk premium on NJs to protect 
          // against bad contained NJs.
          // The code below makes sure we are dealing with joins in the inner.
             if ( joinExpr->child(1).getGroupAttr()-> getNumBaseTables() == 1 )
               return FALSE;
          }
        }
    }

  //------------

  // dont use a nested join in there are more than 3 tables
  // in the inner side (i.e. the right side)
  GroupAnalysis *grpA1 = ((Join *)expr)->child(1).getGroupAttr()->getGroupAnalysis();
  if (grpA1)
    if (grpA1->getAllSubtreeTables().entries() >
        (ULng32)(ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_10))
      return FALSE;
  else
    if ((((Join *)expr)->child(1).getGroupAttr()->getNumBaseTables() >
         (ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_10)) &&
        (CmpCommon::getDefault(COMP_BOOL_112) == DF_OFF))
      return FALSE;

  if (CURRSTMT_OPTDEFAULTS->optimizerHeuristic4() && context
      && context->getInputLogProp()->getResultCardinality() <= 1
      && joinExpr->getSource() != Join::STAR_FACT) // fact table join in star schema
                                                   // is exempt from NJ heuristic pruning
  {
    CostScalar cardinality0 =
      expr->child(0).getGroupAttr()->getResultCardinalityForEmptyInput();
    CostScalar maxCardinality0 =
      expr->child(0).getGroupAttr()->getResultMaxCardinalityForEmptyInput();

    // start avoiding nested join when NJprobeTimeInSec > HJscanTimeInSec
    //   NJprobeTimeInSec = outerRows * 20 * 10 ** -6
    //   HJscanTimeInSec = (innerSize / (1024 * 1024)) / 100
    // so, avoid nested join when
    //   outerRows * 20 * (10 ** -6) > (innerSize / (1024 * 1024)) / 100
    //   outerRows * 20 * (10 ** -6) * 100 * 1024 * 1024 > innerSize
    //   outerRows * 2097 > innerSize
    // 2097 or 2000 is a very rough ratio that can change over time
    // so we take it as a CQD setting
    Lng32 ratio = (ActiveSchemaDB()->getDefaults()).getAsLong
      (HJ_SCAN_TO_NJ_PROBE_SPEED_RATIO);

    if ( CURRSTMT_OPTDEFAULTS->isHashJoinConsidered() AND ratio > 0 AND
         !(joinExpr->getGroupAttr()->isSeabaseMDTable()) )
      {
        // hash join is clearly faster if reading inner table once (after
        // applying local predicates) is faster than reading inner table P
        // times (after applying local predicates + join predicate on
        // inner table's clustering key columns) where P = #outerRows.
        NodeAnalysis *nodeA1 = grpA1->getNodeAnalysis();
        TableAnalysis *tabA1 = NULL;
        if (nodeA1 != NULL && (tabA1=nodeA1->getTableAnalysis()) != NULL
            // We must allow NJ if #outerRows <= 1 because
            // HashJoinRule::topMatch disables HJ for that case.
            // Otherwise, we can get internal error 2235 "pass one
            // skipped, but cannot produce a plan in pass two".
            && cardinality0 > 1) // allow NJ if #outerRows <= 1
          {
            // innerS is size of data scanned for inner table in HJ plan
            CostScalar innerS =
              tabA1->getCardinalityAfterLocalPredsOnCKPrefix() *
              tabA1->getRecordSizeOfBaseTable();

            // if innerS < #outerRows * ratio, prefer HJ over NJ.
            if (innerS < cardinality0 * ratio)
              {
                return FALSE;
              }
            // innerS and #outerRows are estimates that can be wrong.
            // We want to avoid catastrophic nested joins without
            // missing too many performance-enhancing nested joins.
            // A catastrophic nested join is one whose actual #outerRows
            // is much greater than cardinality0. A performance-enhancing
            // nested join is one whose actual #outerRows is much less than
            // cardinality0. Don't consider NJ if
            //   innerS * fudgeFactor < maxCard(outer) * ratio
            double fudge = CURRSTMT_OPTDEFAULTS->robustHjToNjFudgeFactor();
            if (fudge > 0 &&
                innerS * fudge < maxCardinality0 * ratio)
              {
                return FALSE;
              }
          }
      }

    // This is temporary to fail nested join if both children
    // do not have statistics.
    //if ( (CmpCommon::getDefault(COMP_BOOL_67) == DF_ON) AND
    //     (cardinality0 == 100 ) AND (cardinality1 == 100) )
    //  return FALSE;
  }
  //------------

  // if Hash Joins are disabled and we do not have equi-join
  // predicates merge joins will not be selected, so let's
  // enable nested joins
  if (NOT CURRSTMT_OPTDEFAULTS->isHashJoinConsidered() AND
      joinExpr->getEquiJoinPredicates().isEmpty())
     return TRUE;
  else
    // If nested joins are not allowed then say no.
    if (NOT CURRSTMT_OPTDEFAULTS->isNestedJoinConsidered())
      return FALSE;

  // nested Join is only used for cross products if the default
  // NESTED_JOIN_FOR_CROSS_PRODUCTS is ON
  if (joinExpr->isCrossProduct() AND
      NOT CURRSTMT_OPTDEFAULTS->nestedJoinForCrossProducts())
    return FALSE;

  // nested join is not good for filtering joins in the star join schema
  if (CmpCommon::getDefault(COMP_BOOL_85) == DF_OFF &&
      (joinExpr->getSource() == Join::STAR_FILTER_JOIN) &&
      !(joinExpr->isRoutineJoin()))
    return FALSE;

  return TRUE;
}

RelExpr * JoinToTSJRule::nextSubstitute (RelExpr * before,
                                         Context * /*context*/,
                                         RuleSubstituteMemory *& memory)
{
  Join     * bef = (Join *) before;
  // if nested join execluded by heuristics
  //if (NOT bef->canTransformToTSJ()) return NULL;
  OperatorTypeEnum tsjOpType;

  tsjOpType = bef->getTSJJoinOpType();

  // Make sure we call the copyTopNode for the Join.
  Join *result = (Join *)Rule::nextSubstitute(before,NULL,memory);
  result->setOperatorType(tsjOpType);

  // If the join came from a RoutineJoin, remember that.
  if (bef->derivedFromRoutineJoin()) 
    result->setDerivedFromRoutineJoin();

  // now set the group attributes of the result's top node
  result->setGroupAttr(before->getGroupAttr());

  // transfer all the join fields to the new NestedJoin
  (void) bef->copyTopNode(result,CmpCommon::statementHeap());

  result->resolveSingleColNotInPredicate();

  // Recompute the input values that each child requires as well as
  // the output values that each child is capable of producing
  result->allocateAndPrimeGroupAttributes();

  // Push down as many full or partial expressions as can be
  // computed by the children.
  result->pushdownCoveredExpr(
       result->getGroupAttr()->getCharacteristicOutputs(),
       result->getGroupAttr()->getCharacteristicInputs(),
       result->selectionPred());

  // If the right child does not have any predicates on its filter,
  // then this is a cartesian product.  Cartesian products are
  // best implemented as hash joins, so just return NULL.
  RelExpr *filter = result->child(1);

  if (NOT bef->canTransformToTSJ()
      // QSTUFF
      AND NOT result->getGroupAttr()->isEmbeddedUpdateOrDelete()
      AND NOT result->getGroupAttr()->isStream()
      // QSTUFF
      )
  {
    filter->contextInsensRules() = *GlobalRuleSet->applicableRules();
  }

  // Eliminate Filter node if no predicates were added to it
  if (filter->selectionPred().isEmpty())
    {
      result->child(1) = filter->child(0);
      filter->deleteInstance();
      filter = NULL;
    }

  // Eliminate Filter node if no NEW predicates were added to it
  if (filter)
    {
      // Check to see if the filter node is adding any new characteristic
      // inputs (i.e. outer references).  If not, no true join predicates were
      // pushed down.  So, eliminate this filter.
      if( (filter->getGroupAttr()->getCharacteristicInputs() ==
            filter->child(0).getGroupAttr()->getCharacteristicInputs()) AND
          (filter->getGroupAttr()->getCharacteristicOutputs() ==
            filter->child(0).getGroupAttr()->getCharacteristicOutputs()) )
        {
          result->child(1) = filter->child(0);
          filter->deleteInstance();
          filter = NULL;
        }
    }

  // Allow cartisian products for antiJoins or if Hash joins are
  // disabled
  NABoolean hashJoinsEnable = TRUE;
  if (CmpCommon::getDefault(HASH_JOINS) == DF_OFF)
    hashJoinsEnable = FALSE;

  // nested Join is genrally not used for x-products unless the
  // NESTED_JOIN_FOR_CROSS_PRODUCTS default is ON

    if (NOT CURRSTMT_OPTDEFAULTS->nestedJoinForCrossProducts() AND
        NOT filter AND hashJoinsEnable AND NOT bef->isAntiSemiJoin() AND
        // QSTUFF
        // we really favor nested joins in case of streams and embedded
        // embedded updates, so lets retain them
        NOT result->getGroupAttr()->isStream() AND
        NOT result->getGroupAttr()->isEmbeddedUpdateOrDelete()
        // QSTUFF
        )
    {
      result->deleteInstance();
      return NULL;
    }

  // synthesize logical properties for my new right child
  if (filter)
    {
      filter->synthLogProp();
      filter->getGroupAttr()->addToAvailableBtreeIndexes(
        filter->child(0).getGroupAttr()->getAvailableBtreeIndexes());

      if (before->getInliningInfo().isMaterializeNodeForbidden())
        filter->getInliningInfo().setFlags(II_MaterializeNodeForbidden);

      // if the new join is probe cacheable set the right child as probe cacheable too
      if (((NestedJoin*)result)->isProbeCacheApplicable(EXECUTE_IN_MASTER_OR_ESP))
        result->child(1).getGroupAttr()->setIsProbeCacheable();
    }

  // First retain a copy of the equal join expression so that OCR can
  // use it to check if the equi join expression completely covers
  // right child's partitioning key.
  result->setOriginalEquiJoinExpressions(bef->getEquiJoinExpressions());

  // For a TSJ all join predicates have been push down to the
  // second child. We need to clear the equiJoinPredicates.

  result->findEquiJoinPredicates();

  // No need to apply transformation rules on TSJ (since they
  // have already been applied to the JOIN node).  Therefore,
  // mark that we have already tried all transformation rules.
  // For transformation rules that look beyond the top node (ex. left-shift
  // rule), we provide an additional method isTransformComplete() to
  // indicate that this subtree should not be transformed further.
  result->contextInsensRules() += GlobalRuleSet->transformationRules();
  result->setTransformComplete();

  // If the before join resulted from a application of MJPrimeTableRule, directly
  // or indirectly, we set the result join to be also from MJPrimeTableRule.
  if (bef->isJoinFromPTRule())
    result->setJoinFromPTRule();

  return result;
}

Int32 JoinToTSJRule::promiseForOptimization(RelExpr * ,
                                            Guidance * ,
                                            Context * )
{
  // Transforming logical joins to logical TSJs enable the physical
  // NestedJoin rule to fire.  Thus, we would like this rule to fire
  // prior to other join transformation rules.
  return (Int32)(DefaultTransRulePromise - 1000 /*+ 1000*/);
}

NABoolean JoinToTSJRule::canBePruned(RelExpr * expr) const
{
  // We do not want to prune J->TSJ if expr is an AntiSemiJoin
  // since thats its only possible implementation in many cases
  return NOT expr->getOperator().match(REL_ANY_ANTI_SEMIJOIN);
}

// -----------------------------------------------------------------------
// methods for TSJFlow rule
// -----------------------------------------------------------------------
NABoolean TSJFlowRule::topMatch (RelExpr * expr,
                                 Context * context)
{
  if (NOT Rule::topMatch(expr, context))
    return FALSE;

  // We can come here for a CALL statement also, ensure that
  // this rule is not fired for CALL
  if ( REL_CALLSP == expr->getOperatorType ())
      return FALSE;

  GenericUpdate * updateExpr = (GenericUpdate *) expr;
  if (updateExpr->getNoFlow())
    return FALSE;

  // Sol 10-091202-6798, do not try cursor_delete plan if:
  // A. comp_int_74 = 1 (default value is 0) AND rightChild is a DELETE operator
  // B. table being scanned is same as table being deleted.
  // C. scan doesn't have any alternate index access paths.
  // Does it meet condition A?.
  if ( ((ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_74) == 1) AND
        (updateExpr->getOperatorType() == REL_UNARY_DELETE) )
  {
    Delete * del = (Delete *) expr;
    RelExpr* c0= expr->child(0).getGroupAttr()->getLogExprForSynthesis();

    if ( c0->getOperatorType() != REL_SCAN )
       return TRUE;

    Scan * scan = (Scan *)c0;

    // Does it meet condition B?.
    // check if the table scanned is same as that being deleted.
    if ( (scan != NULL) AND
          scan->getTableDesc()->getNATable() == del->getTableDesc()->getNATable())
    {
      // go through all indexes and find out in which way they could
      // be used, if this is not already done (needed for next step)
      scan->addIndexInfo();

      // Does it meet condition C?.
      // don't apply the rule if there isn't at least one alternate index.
      // cursor_delete doesn't make any sense, subset_delete is better.
      if ((scan->getIndexOnlyIndexes().entries() +
           scan->getPossibleIndexJoins().entries()) < 2)
        return FALSE;
    }
  }

  return TRUE;
}

// ##IM: ?? This rule is not calling pushdownCoveredExpr.  Should it?
RelExpr * TSJFlowRule::nextSubstitute (RelExpr * before,
                                       Context * /*context*/,
                                       RuleSubstituteMemory *& /*memory*/)
{
  // Create a new "leaf" generic update expression from the old
  // unary generic update expression.
  GenericUpdate * oldUpdate = (GenericUpdate *) before;

  if (oldUpdate->getGroupAttr()->getCharacteristicOutputs().entries())
  {
    return NULL;
  }

  RelExpr * newUpdate = oldUpdate->copyTopNode(0,CmpCommon::statementHeap());
  OperatorTypeEnum newOptype = NO_OPERATOR_TYPE;

  switch (before->getOperatorType())
    {
    case REL_UNARY_INSERT:
      newOptype = REL_LEAF_INSERT;
      break;
    case REL_UNARY_UPDATE:
      newOptype = REL_LEAF_UPDATE;
      break;
    case REL_UNARY_DELETE:
      newOptype = REL_LEAF_DELETE;
      break;
    default:
      ABORT ("Internal error: TSJFlowRule::nextSubstitute");
      break;
    }
  newUpdate->setOperatorType (newOptype);

  // Create the new TSJFlow expression
  Join * result = new (CmpCommon::statementHeap())
    Join(before->child(0),
         newUpdate,
         REL_TSJ_FLOW,
         NULL,
         FALSE,
         FALSE,
         CmpCommon::statementHeap(),
         ((GenericUpdate *) oldUpdate)->getTableDesc(),
         new (CmpCommon::statementHeap()) ValueIdMap(
            ((GenericUpdate *) oldUpdate)->updateToSelectMap()));

  // This TSJ is for a write operation
  result->setTSJForWrite(TRUE);

  // TSJ for self-referencing updates need special treatment
  oldUpdate->configTSJforHalloween( result, newOptype,
        before->child(0).getGroupAttr()->getResultCardinalityForEmptyInput());

  // For Neo R2 compatibility:
  // Transfer the avoidHalloweenR2 flag from the update to the join.  If
  // this flag is set, then we must generate a safe plan WRT the
  // Halloween problem.  For the TSJForWrite Join the plan should have
  // a SORT operator on the LHS of the Tuple Flow.  This causes the
  // source to be blocked.
  //
  result->setAvoidHalloweenR2(oldUpdate->avoidHalloweenR2());

  // Now set the group attributes of the result's top node.
  result->setGroupAttr(before->getGroupAttr());

  // Recompute the input values that each child requires as well as
  // the output values that each child is capable of producing
  result->allocateAndPrimeGroupAttributes();

  // Output values produced by the left child of the tsjFlow
  // becomes the inputs for the right child of the tsjFlow.
  RelExpr * leftChild = result->child(0);
  RelExpr * rightChild = result->child(1);
  rightChild->getGroupAttr()->addCharacteristicInputs
    (((RelExpr *)before->child(0))->getGroupAttr()->getCharacteristicOutputs());

  // Push down as many full or partial expressions as can be
  // computed by the children.
  result->pushdownCoveredExpr(result->getGroupAttr()->getCharacteristicOutputs(),
                              result->getGroupAttr()->getCharacteristicInputs(),
                              result->selectionPred());

  // Synthesize logical properties for this new leaf generic update node.
  ((GenericUpdate *)rightChild)->synthLogProp();

  result->setBlockStmt( before->isinBlockStmt() );

  if (rightChild->getOperatorType() == REL_LEAF_INSERT)
  {
    // if my right child is an insert node, then buffered inserts are
    // allowed. The kind of buffered inserts (VSBB or sidetree) are
    // chosen in DP2InsertCursorRule::nextSubstitute. At that time,
    // buffered inserts may be disabled due to other conditions
    // (See that method).
    Insert * ins
      = (Insert *)(rightChild->castToRelExpr());
    ins->bufferedInsertsAllowed() = TRUE;

    // Transfer any required order from the insert node to the TSJ node.
    // There will only be a req. order if the user specified an ORDER BY clause.
    result->setReqdOrder(ins->reqdOrder());

    // Transfer the sidetree insert indicator to the join.
    //
    // Later on in NJ::createContextForChild(), plans that do not sort
    // the data from the outer side will be disabled.
    //
    // Also transfer sidetree insert flag to OptDefaults so that MUs can use.

    if ( ins->isSideTreeInsertFeasible() ) {
      result->setTSJForSideTreeInsert(TRUE);
      CURRSTMT_OPTDEFAULTS->setSideTreeInsert(TRUE);
    }

    if (ins->enableTransformToSTI())
      result->setEnableTransformToSTI(TRUE);

    result->setIsForTrafLoadPrep(ins->getIsTrafLoadPrep());
  }

  return result;
}

// -----------------------------------------------------------------------
// methods for TSJ rule
// -----------------------------------------------------------------------
NABoolean TSJRule::topMatch (RelExpr * expr,
                             Context * context)
{
  if (NOT Rule::topMatch(expr, context))
    return FALSE;

  // We can come here for a CALL statement also, ensure that
  // this rule is not fired for CALL
  if ( REL_CALLSP == expr->getOperatorType ())
      return FALSE;

  GenericUpdate * updateExpr = (GenericUpdate *) expr;
  if (NOT updateExpr->getNoFlow() || 
      // no FirstN on top of a TSJ
      ((updateExpr->getOperatorType() == REL_UNARY_DELETE) && updateExpr->isMtsStatement()))
    return FALSE;

  // It is not semantically correct to convert a MERGE having a 
  // "NOT MATCHED" action to a TSJ, since the former has right 
  // join semantics. (If we converted here to a TSJ, a non-matching
  // row would not be returned by the outer child scan node, so
  // the inner child merge node would never see it and hence the
  // "NOT MATCHED" logic would not be activiated.)
  if (updateExpr->isMerge() && updateExpr->insertValues())
    return FALSE;

  return TRUE;
}

//
//      Right now the VSBBInsert's are disabled in the
//      DP2InsertCursorRule::nextSubstitute() method if there are any
//      indexes on the table.  With the new IM code we need to enable
//      VSBB inserts for the original insert which is on the left subtree
//      and test inserts.  With the new tranformation rule if the insert
//      has output values it will choose NJ as the parent of the insert.
//      But if optimizer chooses the insert to be a VSBBInsert then the
//      combination of VSBBInsert with parent as NJ would not work.
//      Because VSBBInsert always assumes that the parent is a TupleFlow.
//      So may be the new rule may not be right.  In that case we need
//      to make changes to the TupleFlow in the executor to return rows
//      and remove the new rule.
//
//      Also:
//        Enabling VSBBInserts when there are indexes on the table,
//        Enabling Subsets for Delete's and Update's and testing.
//
// ##IM: ?? This rule is not calling pushdownCoveredExpr.  Should it?
//
RelExpr * TSJRule::nextSubstitute (RelExpr * before,
                                   Context * /*context*/,
                                   RuleSubstituteMemory *& /*memory*/)
{
  // Create a new "leaf" generic update expression from the old
  // unary generic update expression.
  GenericUpdate * oldUpdate = (GenericUpdate *) before;
  RelExpr * oldExpr = before->child(0).getPtr();

  RelExpr * newUpdate = oldUpdate->copyTopNode(0,CmpCommon::statementHeap());
  OperatorTypeEnum newOptype = NO_OPERATOR_TYPE;

  switch (before->getOperatorType())
    {
    case REL_UNARY_INSERT:
      newOptype = REL_LEAF_INSERT;
      break;
    case REL_UNARY_UPDATE:
      newOptype = REL_LEAF_UPDATE;
      break;
    case REL_UNARY_DELETE:
      newOptype = REL_LEAF_DELETE;
      break;
    default:
      ABORT ("Internal error: TSJRule::nextSubstitute");
      break;
    }
  newUpdate->setOperatorType (newOptype);

  // Create the new TSJ expression

  Join * result = new (CmpCommon::statementHeap())
    Join(before->child(0),
         newUpdate,
         REL_TSJ,
         NULL,
         FALSE,
         FALSE,
         CmpCommon::statementHeap(),
         oldUpdate->getTableDesc(),
         new (CmpCommon::statementHeap()) ValueIdMap(
            oldUpdate->updateToSelectMap()));


  // This TSJ is for a write operation
  result->setTSJForWrite(TRUE);

  // Do not allow this TSJ to run inside DP2 if it is created for a MV INSERT
  // to avoid possible DP2 starvation (e.g., when the source is a SELECT).
  if ((before->getInliningInfo()).isMVLoggingInlined() == TRUE)
     result->setAllowPushDown(FALSE);

  // TSJ for self-referencing updates need special treatment.
  oldUpdate->configTSJforHalloween( result, newOptype,
        before->child(0).getGroupAttr()->getResultCardinalityForEmptyInput());

  // Now set the group attributes of the result's top node.
  result->setGroupAttr(before->getGroupAttr());

  // QSTUFF
  // in case we force a cursor update to get a place holder
  // for the outer selection predicates pushed down to the
  // generic update tree we have to set the selection
  // predicates at the new root of the generic update tree
  if (result->getGroupAttr()->isGenericUpdateRoot())
                result->selectionPred() += before->getSelectionPred();
  // QSTUFF

  // Recompute the input values that each child requires as well as
  // the output values that each child is capable of producing
  result->allocateAndPrimeGroupAttributes();

  RelExpr * leftChild = result->child(0);
  RelExpr * rightChild = result->child(1);

  // QSTUFF
  // a leaf can only produce new or old values in case of an update or delete
  // but the copyTopNode method has set the potentialOutputs to both the new
  // and the old values. This causes primeGroupAttr to set the characteristic
  // outputs of the leaf operator to both the old and new values, this causes
  // problems in precodegen as precode gen does not find any of them
  // being produced.
  // We therefore  set the characteristic output of the new leaf to what the
  // old unary operator could produce..and pray..
  // for a final solution one would have to correct the way potentialOutputs
  // are set
  rightChild->getGroupAttr()->setCharacteristicOutputs
  (((RelExpr *)before)->getGroupAttr()->getCharacteristicOutputs());
  // QSTUFF

  // In the case of an embedded insert with a selection predicate,
  // we need to retrieve the stored available outputs
  // from the GenericUpdate group attr.

  if (result->getGroupAttr()->isEmbeddedInsert() &&
      !result->selectionPred().isEmpty() &&
      rightChild->getArity() >0)
    {
      ValueIdSet availableGUOutputs;
      rightChild->child(0)->getInputAndPotentialOutputValues(availableGUOutputs);
      result->getGroupAttr()->addCharacteristicOutputs(availableGUOutputs);
    }  // End of embedded insert setting of characteristic outputs


  rightChild->getGroupAttr()->addCharacteristicInputs
  (((RelExpr *)before->child(0))->getGroupAttr()->getCharacteristicOutputs());

  // Synthesize logical properties for this new leaf generic update node.
  ((GenericUpdate *)rightChild)->synthLogProp();

  // QSTUFF
  // we need the unique properties to be set
  result->setCursorUpdate(TRUE);
  // QSTUFF

  result->setBlockStmt( before->isinBlockStmt() );

  if (rightChild->getOperatorType() == REL_LEAF_INSERT)
  {
    Insert * ins
      = (Insert *)(rightChild->castToRelExpr());

	// MV -
	// Allow VSBB Inserts even when its outputs are used for IM etc.
    ins->bufferedInsertsAllowed() = TRUE;

    // Transfer any required order from the insert node to the TSJ node.
    // There will only be a req. order if the user specified an ORDER BY clause.
    result->setReqdOrder(ins->reqdOrder());

    result->setIsForTrafLoadPrep(ins->getIsTrafLoadPrep());
  }


  return result;
}

// -----------------------------------------------------------------------
// methods for FilterRule
// -----------------------------------------------------------------------

Guidance * FilterRule::guidanceForExploringChild(Guidance *,
                                                  Context *,
                                                  Lng32)
{
  return new (CmpCommon::statementHeap())
    FilterGuidance(CmpCommon::statementHeap());
}

RelExpr * FilterRule0::nextSubstitute(RelExpr * before,
                                      Context *,
                                      RuleSubstituteMemory * &)
{
  RelExpr * result;
  Filter  * bef = (Filter *) before;
  RelExpr * leafNode = before->child(0);

  CMPASSERT(before->getOperatorType() == REL_FILTER);

  // copy the tree underneath the filter
  result = getSubstitute()->copyTree(CmpCommon::statementHeap());

  // if the original child of the filter is a scan, then also copy
  // index information to prevent enumerating potential index joins again.
  if ((leafNode->getOperatorType() == REL_SCAN))
    ((Scan *)result)->copyIndexInfo(leafNode);

  // now set the group attributes of the result's top node
  result->setGroupAttr(before->getGroupAttr());

  result->selectionPred() += bef->selectionPred();

  return result;
}

RelExpr * FilterRule1::nextSubstitute(RelExpr * before,
                                      Context * /*context*/,
                                      RuleSubstituteMemory * &)
{
  RelExpr * result;
  RelExpr * unaryNode = before->child(0);
  ValueIdSet newPredicates;

  CMPASSERT(before->getOperatorType() == REL_FILTER);

  // copy the tree underneath the filter
  result = getSubstitute()->copyTree(CmpCommon::statementHeap());

  // If the child of the filter is itself a filter, then just return NULL.
  // Since children groups are explored first, allow the child filter
  // node to be collapsed first prior to applying this rule.
  //
  // NOTE: The above statement is not true. Due to a bug in Cascades,
  // the child filter group is NOT explored first. An attempt was
  // made to get this to work by adding a kind of guidance called
  // "FilterGuidance". This caused worse problems. So, the two lines
  // below that return if the child of the filter is a filter were
  // removed. This caused worse problems, too. An attempt was made
  // to solve those problems but that caused even worse problems.
  // So, for now, we have to live with the original problem, which
  // is documented in CR# 10-001006-2815.  To learn about the
  // "worse" problems and the attempted fixes, see CR# 10-001204-9989
  // and CR# 10-010104-0497.
  if (result->getOperatorType() == REL_FILTER)
    return NULL;

  // now set the group attributes of the result's top node
  result->setGroupAttr(before->getGroupAttr());

  // Find out if there are any new predicates to push down
  newPredicates = before->selectionPred();
  newPredicates -= result->selectionPred();

  // NOTE: even if there are no new predicates, if the char. inputs of
  // the result node are different from the char. inputs of the
  // original node "unaryNode", then we still need to perform
  // predicate pushdown. This is because additional inputs can change the
  // meaning of a VEGPredicate in a node (instead of being an IS NOT NULL
  // predicate, the predicate now compares two VEG members).

  if (NOT newPredicates.isEmpty() OR
      unaryNode->getGroupAttr()->getCharacteristicInputs() !=
      result->getGroupAttr()->getCharacteristicInputs())
    {
      // push down to the filter node below result as many full or
      // partial expressions as can be computed by the (Filter) child.
      result->selectionPred() += newPredicates;

      // recompute the input values that each child of result requires
      // as well as the output values that each child is capable of
      // producing.
      result->allocateAndPrimeGroupAttributes();

      // do the pushdown, which has the side-effect of recomputing
      // the correct characteristic inputs and outputs of the child
      result->pushdownCoveredExpr(
               result->getGroupAttr()->getCharacteristicOutputs(),
               result->getGroupAttr()->getCharacteristicInputs(),
               result->selectionPred());
    }

  // If no predicates were placed on the filter
  // node below result then remove the filter node.
  Filter * filter = (Filter *)(result->child(0)->castToRelExpr());
  if (((Filter *)before)->reattemptPushDown()) filter->setReattemptPushDown();
  if (filter->selectionPred().isEmpty())
  {
    result->child(0) = filter->child(0).getPtr();
    filter->deleteInstance();
  }
  else // new filter has predicates
  {
    // If no new predicates could be pushed to the filter node
    // delete it.
    if( (filter->getGroupAttr()->getCharacteristicInputs() ==
          filter->child(0).getGroupAttr()->getCharacteristicInputs()) AND
        (filter->getGroupAttr()->getCharacteristicOutputs() ==
          filter->child(0).getGroupAttr()->getCharacteristicOutputs()) AND
        (NOT filter->reattemptPushDown()) )
    {
      result->child(0) = filter->child(0).getPtr();
      filter->deleteInstance();
    }
    else // new filter predicates are new
    {
      // If the original unary node was a filter, get rid of it. It
      // should no longer be necessary, since we are saving the new one.
      if (result->getOperatorType() == REL_FILTER)
      {
        Filter* resultAsFilter = (Filter *)(result->castToRelExpr());
        // Make sure the unary node filter - i.e. the original
        // lower level filter - gave all it's predicates to the
        // filter we just pushed down.
        DCMPASSERT(resultAsFilter->selectionPred().isEmpty());
        // Get rid of the original lower level filter node by setting
        // the result to it's child.
        result = filter;
        resultAsFilter->deleteInstance();
      }
      // synthesize logical properties for this new node.
      filter->synthLogProp();
    } // end if new filter preds are new
  } // end if new filter has predicates

  return result;
}


RelExpr * FilterRule2::nextSubstitute(RelExpr * before,
                                      Context * /*context*/,
                                      RuleSubstituteMemory * &)
{
  RelExpr * result;
  RelExpr * binaryNode = before->child(0);
  ValueIdSet newPredicates;
  NABoolean  recomputeEquiJoinPred = FALSE;  //Sol no:10-030731-8378


  // Solution 10-031107-1132: due to a group merge, Filter and Filter(cut)
  // may end up in the same group. In that case, do not apply this rule
  if ((before->getGroupId() != INVALID_GROUP_ID) &&
      (before->getGroupId() == binaryNode->getGroupId())
     )
    return NULL;

  CMPASSERT(before->getOperatorType() == REL_FILTER);

  // copy the tree underneath the filter
  result = getSubstitute()->copyTree(CmpCommon::statementHeap());

  // now set the group attributes of the result's top node
  result->setGroupAttr(before->getGroupAttr());

  RelExpr * leftChild = result->child(0);
  RelExpr * rightChild = result->child(1);

  // recompute the input values that each child of result requires
  // as well as the output values that each child is capable of
  // producing.
  result->allocateAndPrimeGroupAttributes();

  // Find out if there are any new predicates to push down
  newPredicates = before->selectionPred();
  newPredicates -= result->selectionPred();

  // NOTE: even if there are no new predicates, if the char. inputs of
  // the result node are different from the char. inputs of the
  // original node "binaryNode", then we still need to perform
  // predicate pushdown. This is because additional inputs can change the
  // meaning of a VEGPredicate in a node (instead of being an IS NOT NULL
  // predicate, the predicate now compares two VEG members).

  if (NOT newPredicates.isEmpty() OR
      binaryNode->getGroupAttr()->getCharacteristicInputs() !=
      result->getGroupAttr()->getCharacteristicInputs())
  {
    // push down to the filter node below result as many full or
    // partial expressions as can be computed by the (Filter) child.
    result->selectionPred() += newPredicates;
    result->pushdownCoveredExpr(
               result->getGroupAttr()->getCharacteristicOutputs(),
               result->getGroupAttr()->getCharacteristicInputs(),
               result->selectionPred() );

    // If the binary node in the before pattern is a Join, then we
    // need to recompute the equijoin predicates in the result, since
    // we now have changed the predicates attached to the result.
    // We also need to set the joinFromPTRule_ flag of result if set
    // in the binaryNode
    // Sol no:10-030731-8378-> commented out the equi-join
    // check.Do it after the removal of the redundant filters, if any.
    recomputeEquiJoinPred = TRUE;
    //<-Sol no:10-030731-8378
  }

  if (binaryNode->getOperator().match(REL_ANY_JOIN))
    {
      // If the binaryNode join resulted from a application of MJPrimeTableRule, directly
      // or indirectly, we set the result join to be also from MJPrimeTableRule.
      if (((Join *)binaryNode)->isJoinFromPTRule())
        ((Join *)result)->setJoinFromPTRule();
    }

  // If no predicates were placed on the filter nodes below result
  // then remove them else synthesis their logical properties.
  Filter * filter = (Filter *)(result->child(0)->castToRelExpr());
  if (((Filter *)before)->reattemptPushDown()) filter->setReattemptPushDown();
  if (filter->selectionPred().isEmpty())
    {
      result->child(0) = filter->child(0).getPtr();
      filter->deleteInstance();
    }
  else
    {
      // If no new predicates could be pushed to the filter node
      // delete it.
      if( (filter->getGroupAttr()->getCharacteristicInputs() ==
            filter->child(0).getGroupAttr()->getCharacteristicInputs()) AND
          (filter->getGroupAttr()->getCharacteristicOutputs() ==
            filter->child(0).getGroupAttr()->getCharacteristicOutputs()) AND
          (NOT filter->reattemptPushDown()) )
        {
          result->child(0) = filter->child(0).getPtr();
          filter->deleteInstance();
        }
      else
        // synthesize logical properties for this new node.
        filter->synthLogProp();
    }

  filter = (Filter *)(result->child(1)->castToRelExpr());
  if (((Filter *)before)->reattemptPushDown()) filter->setReattemptPushDown();
  if (filter->selectionPred().isEmpty())
    {
      result->child(1) = filter->child(0).getPtr();
      filter->deleteInstance();
    }
  else
    {
      // If no new predicates could be pushed to the filter node
      // delete it.
      if( (filter->getGroupAttr()->getCharacteristicInputs() ==
            filter->child(0).getGroupAttr()->getCharacteristicInputs()) AND
          (filter->getGroupAttr()->getCharacteristicOutputs() ==
            filter->child(0).getGroupAttr()->getCharacteristicOutputs()) AND
          (NOT filter->reattemptPushDown()) )
        {
          result->child(1) = filter->child(0).getPtr();
          filter->deleteInstance();
        }
      else
        // synthesize logical properties for this new node.
        filter->synthLogProp();
    }

  //Sol no:10-030731-8378->
  if (recomputeEquiJoinPred)
  {
    if (binaryNode->getOperator().match(REL_ANY_JOIN))
      ((Join *)result)->findEquiJoinPredicates();
  }
  //<-Sol no:10-030731-8378
  return result;
}

Int32 FilterRule::promiseForOptimization(RelExpr *,
                                         Guidance *,
                                         Context *)
{
  // Eliminating the filter has the highest promise, higher than
  // any enforcer rule (only enforcer rules compete with this rule anyway)
  return AlwaysBetterPromise;
}

// -----------------------------------------------------------------------
// methods for class GroupByMVQRRule
// -----------------------------------------------------------------------

NABoolean GroupByMVQRRule::topMatch(RelExpr * expr,
                                    Context * context)
{
  if (NOT Rule::topMatch(expr,context))
    return FALSE;

  if ((expr->getOperatorType() != REL_GROUPBY) && (expr->getOperatorType() != REL_AGGREGATE))
    return FALSE;

  // For optimization levels below the medium level we do not run the GroupByMVQRRule
  if (CURRSTMT_OPTDEFAULTS->optLevel() < OptDefaults::MEDIUM)
    return FALSE;

  // rule is disabled
  if (CmpCommon::getDefaultLong(MVQR_REWRITE_LEVEL) < 1)
    return FALSE;

  GroupByAgg *grby = (GroupByAgg *) expr;

  // ---------------------------------------------------------------------
  // If this GroupByAgg was created by the GroupBySplitRule, no
  // further transformation rules should be applied to it. 
  // ---------------------------------------------------------------------
  if (NOT grby->isNotAPartialGroupBy())
    return FALSE;

  // ---------------------------------------------------------------------
  // If this GroupByAgg was created by the AggrDistinctEliminationRule or
  // GroupByOnJoinRule, no further transformation rules should be 
  // applied to it.
  // ---------------------------------------------------------------------
  if (grby->aggDistElimRuleCreates() || grby->groupByOnJoinRuleCreates())
    return FALSE;

  // Any matching MVs for this JBBsubset
  JBBSubsetAnalysis* grbyJBBSubsetAnalysis =  grby->getJBBSubsetAnalysis();
  if (grbyJBBSubsetAnalysis &&
      grbyJBBSubsetAnalysis->getMatchingMVs().entries() &&
      !(grbyJBBSubsetAnalysis->getMatchingMVs()[0]->alreadyOptimized()))
	return TRUE;  

  return FALSE;  
}

RelExpr * GroupByMVQRRule::nextSubstitute(RelExpr * before,
                                          Context * /*context*/,
                                          RuleSubstituteMemory * & memory)
{
  RelExpr *result = NULL;

  CMPASSERT((before->getOperatorType() == REL_GROUPBY) || (before->getOperatorType() == REL_AGGREGATE));

  GroupByAgg *grby = (GroupByAgg *) before;

  if (memory == NULL)
  {
      // -----------------------------------------------------------------
      // this is the first call, create info on all matches
      // -----------------------------------------------------------------

     JBBSubsetAnalysis*   jbbSubsetAnal = grby->getGBAnalysis()->getJBB()->getMainJBBSubset().getJBBSubsetAnalysis();

     CollIndex numMatches = jbbSubsetAnal->getMatchingMVs().entries();

     if (numMatches)
     {
        // allocate a new memory for multiple substitutes
        memory = new (CmpCommon::statementHeap())
          RuleSubstituteMemory(CmpCommon::statementHeap());

        for (CollIndex matchIndex = 0; matchIndex < numMatches; matchIndex++)
        {
             // get the next match
             MVMatchPtr match = jbbSubsetAnal->getMatchingMVs()[matchIndex];

             match->setAlreadyOptimized();
   
             RelExpr *matchExpr = match->getMvRelExprTree();
   
             // now set the group attributes of the result's top node
             matchExpr->setGroupAttr(before->getGroupAttr());
   
             // insert the match into the substitute memory
             memory->insert(matchExpr);
        } // for each match
     } // if numMatches
  } // memory == NULL

  // ---------------------------------------------------------------------
  // handle case of multiple substitutes
  // ---------------------------------------------------------------------
  if (memory != NULL)
  {
      result = memory->getNextSubstitute();

      if (result == NULL)
      {
          // returned all the substitutes
          // now delete the substitute memory, so we won't be called again
          delete memory;
          memory = NULL;
      }
      else
      {
         // synth the MVI
         result->synthLogProp();
      }

      // return the next retrieved substitute
      return result;
  }
  else
    return NULL; // rule didn't fire
}

// -----------------------------------------------------------------------
// methods for class GroupByEliminationRule
// -----------------------------------------------------------------------

GroupByEliminationRule::~GroupByEliminationRule() {}

NABoolean GroupByEliminationRule::topMatch(RelExpr * expr,
                                           Context * context)
{
  if (NOT Rule::topMatch(expr,context))
    return FALSE;

  if (expr->getOperatorType() == REL_SHORTCUT_GROUPBY)
    return FALSE;

  // make sure we are looking at a group by node
  CMPASSERT(expr->getOperatorType() == REL_GROUPBY);

  GroupByAgg *grby = (GroupByAgg *) expr;

  // ---------------------------------------------------------------------
  // If this GroupByAgg was created by the GroupBySplitRule, no
  // further transformation rules should be applied to it. The
  // GroupBySplitRule replaces GB(CutOp) with a GB(GB(CutOp)).
  // Our intention is to apply the relevant transformation rules
  // to the original pattern but not to its substitute so that each
  // GroupByAgg in the substitute is subjected to implementation
  // rules only.
  // ---------------------------------------------------------------------
  if (NOT grby->isNotAPartialGroupBy()) // do not apply to partial gb
    return FALSE;

  // We do NOT eliminate any aggregate nodes (nodes without groupby columns),
  // since that would cause problems with the case of an empty input
  // and since it wouldn't save any cost.
  if (grby->groupExpr().isEmpty())
    return FALSE;

  // do not eliminate group by if rollup is being done
  if (grby->isRollup())
    return FALSE;

  return (grby->child(0).getGroupAttr()->isUnique(grby->groupExpr()));
}

Int32 GroupByEliminationRule::promiseForOptimization(RelExpr *,
                                                     Guidance *,
                                                     Context *)
{
  // give this highest priority, no groupby is always faster than a groupby
  return AlwaysBetterPromise;
}

RelExpr * GroupByEliminationRule::nextSubstitute(RelExpr * before,
                                                 Context * /*context*/,
                                                 RuleSubstituteMemory * & memory)
{
  // the substitute in the rule is simply a Cut operator
  RelExpr *result = getSubstitute()->copyTree(CmpCommon::statementHeap());

  CMPASSERT(before->getOperatorType() == REL_GROUPBY);
  GroupByAgg *grby = (GroupByAgg *) before;

  // ---------------------------------------------------------------------
  // Make four different substitutes, depending on whether expressions
  // need to be rewritten and whether there are HAVING predicates:
  //
  // a)   cut        ...if no expressions need to be rewritten, if
  //                    there are no HAVING predicates, and if the group
  //                    attributes of the result are the same as those
  //                    of the cut
  //
  // b)  filter      ...if there is a HAVING predicate, but no expressions
  //       |            need to be rewritten, OR if the cut operator's group
  //      cut           attributes are not the same as the result's group
  //                    attributes,
  //
  // c)   mvi        ...if there is no HAVING predicate, but expressions
  //       |            need to be rewritten,
  //      cut
  //
  // d)   mvi        ...you guessed it: if there are HAVING predicates
  //       |            and if expressions need to be rewritten.
  //     filter
  //       |
  //      cut
  //
  // ---------------------------------------------------------------------

  // If there are no aggregates, then we know that no expressions need to
  // be rewritten and that we can safely return the cut node (or a
  // node with the same group attributes) as the result.
  if (grby->aggregateExpr().isEmpty())
    {
      // if there is a having predicate or if the group attributes of
      // the child are different from the result's group attributes, then
      // create a filter node on top of the result so far
      if (NOT (grby->selectionPred().isEmpty() AND
               *(grby->getGroupAttr()) == *(grby->child(0).getGroupAttr())))
        {
          // case b)
          result = new (CmpCommon::statementHeap()) Filter(result);
          ((Filter *)result)->setReattemptPushDown();
          result->selectionPred() = grby->selectionPred();

          // NOTE: the query
          //
          //    select a,b
          //    from t
          //    group by a,b,c
          //
          // where a,b,c is unique is an example for the case
          // where we generate an empty filter node just because
          // the group attributes are different.
        }
      // else /* case a) */
    }
  else
    {

      // Otherwise, create a MapValueIds node, replace the aggregate functions
      // with values that contain the result of an aggregation on a single row
      // (e.g. count(*) ==> 1, sum(x) ==> x, max(x) ==> x), and map the
      // input value ids to the corresponding output value ids.

      MapValueIds *mvi = new (CmpCommon::statementHeap()) MapValueIds(result);

      // rewrite all aggregate functions and remap references to
      // them to their new values
      for (ValueId x = grby->aggregateExpr().init();
           grby->aggregateExpr().next(x);
           grby->aggregateExpr().advance(x))
        {
          CMPASSERT(x.getItemExpr()->isAnAggregate());
          Aggregate *aggrExpr = (Aggregate *) x.getItemExpr();

          mvi->addMapEntry(
               aggrExpr->getValueId(),
               aggrExpr->rewriteForElimination()->getValueId());
        }

      // Remember the original aggregate inputs, which contain information
      // needed to rewrite VEGReferences into actual columns in the generator.
      // NOTE: this might cause some unnecessary expressions to be carried to
      // this node, but the cost for this shouldn't be too high.

      // Removed 10/10/16 as part of fix for TRAFODION-2127
      // These values were not used in MapValueIds::preCodeGen.
      // Could consider adding this if there are issue in preCodeGen.

      // ValueIdSet valuesForRewrite;

      // grby->getValuesRequiredForEvaluatingAggregate(valuesForRewrite);
      // mvi->addValuesForVEGRewrite(valuesForRewrite);

      // If there are having predicates, put a filter below the mvi
      // node. Then map the having predicates and attach them to
      // the filter.
      if (NOT grby->selectionPred().isEmpty())
        {
          // case d)

          // install the a filter below the mvi node
          Filter *filter = new (CmpCommon::statementHeap())
                             Filter(mvi->child(0));
          filter->setReattemptPushDown();
          mvi->setChild(0,filter);

          // Map the HAVING predicate using the map from the newly allocated
          // MapValueIds node. This used to be done by attaching the HAVING
          // predicate to the mvi node and letting pushdown do the mapping,
          // then asserting that the predicate did, in fact, get pushed down.
          // The problem with this is that if the predicate only has outer
          // references and does not reference any columns that are output
          // by the child of the mvi then the predicate will not be pushed
          // down (see VegPredicate::isCovered). An example predicate is
          // t1.vch7 = 'b', where t1.vch7 is an outer reference.
          ValueIdSet selPredsRewritten;
          ValueIdSet selPreds = grby->selectionPred();
          mvi->getMap().rewriteValueIdSetDown(selPreds,selPredsRewritten);
          filter->selectionPred() = selPredsRewritten;

          // allocate group attributes for the filter
          mvi->setGroupAttr(before->getGroupAttr());
          mvi->allocateAndPrimeGroupAttributes();
          mvi->pushdownCoveredExpr(
               mvi->getGroupAttr()->getCharacteristicOutputs(),
               mvi->getGroupAttr()->getCharacteristicInputs(),
               mvi->selectionPred() );

          // perform synthesis on the new filter node
          mvi->child(0)->synthLogProp ();
        }
      // else /* case c) */

      result = mvi;

    }

  // come up with the group attributes for the new result
  result->setGroupAttr(before->getGroupAttr());

  // this rule fires only once
  CMPASSERT(memory == NULL);

  return result;
}

// -----------------------------------------------------------------------
// The GroupBySplitRule
//
// The optimizer views a GroupByAgg to be in one of four forms:
//
// 1) A "full" groupby, i.e., it is not a partial groupby.
//
//    The parser creates a group by of this form. A groupby of this
//    form is seen by all the phases of processing from name binding
//    through code generation.
//
//    The method isNotAPartialGroupBy() returns TRUE for a "full"
//    groupby and false for all other forms.
//
// 2) Partial groupby operators.
//
//    Partial groupby operators can appear in the solution for a
//    group by because they are created by the application of the
//    transformation rule, the GroupBySplitRule. The GroupBySplitRule
//    fires once for a "full" groupby and can fire once more for a
//    partial groupby that is a leaf. The rule replaces
//                    GB(X) with GB(GB(X))
//    Partial groupby operators are only seen by the optimizer, the
//    pre code generator and the code generator.
//
//    When the GroupBySplitRule fires on a "full" groupby it produces
//    the following pairs of groupbys
//
//    a) Root of the partial groupby subtree
//
//       The root of the partial groupby consolidates the partial groups
//       that are formed by one or more groupbys that independently
//       compute partial groups.
//
//       The method isAPartialGroupByRoot() returns TRUE only for such
//       a partial groupby.
//
//    b) 1st Leaf of the partial groupby subtree.
//
//       The 1st leaf of the partial groupby subtree is a simply the
//       the lowermost partial groupby created when the GroupBySplitRule
//       fires on a "full" groupby. The term "leaf" is convenient for
//       naming the partial groupby operators that are contained in the
//       substitute. The reader should realize that, in reality, the
//       GroupByAgg is a unary operator; the partial groupby will always
//       posses a child, regardless of whether it is called a "leaf"!
//
//       The method isAPartialGroupByLeaf1() returns TRUE only for such
//       a partial groupby.
//
//    The GroupBySplitRule can fire on the 1st leaf to produce another
//    pair of parital groupby operators called the non-leaf partial
//    groupby together with the 2nd leaf.
//
//    c) Non-leaf partial groupby
//
//       A plan can contain three partial groupbys that span two
//       levels of the tree, the root, a non-leaf and a leaf.
//       The non-leaf partially consolidates partial groups
//       that are formed by the 2nd leaf of the partial groupby
//       subtree. It adds one more operator to the pipeline but
//       increases the degree of parallelism for the leaves.
//
//       The method isAPartialGroupByNonLeaf() returns TRUE only for such
//       a partial groupby.
//
//    d) 2nd Leaf of the partial groupby subtree.
//
//       The 2nd leaf of the partial groupby subtree is a simply the
//       the lowermost partial groupby created when the GroupBySplitRule
//       fires on a partial groupby that is a 1st leaf.
//
//       The method isAPartialGroupByLeaf2() returns TRUE only for such
//       a partial groupby.
//
// The distinction between the 1st leaf and the 2nd leaf is made purely to
// aid costing in the optimizer. The code generator processes both
// the forms of the leafs in an identical manner. The method
// isAPartialGroupByLeaf() is defined to permit the code generator
// to identify a leaf, independent of its form.
//
// A GroupByAgg operator in any one of the above forms uses the same
// implementation for grouping the data at run time. This means,
// regardless of its form, a GroupByAgg will either use the hash
// grouping or sorted grouping implementation at run time.
//
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// GroupByTernarySplitRule
//
// The following rule is only used for performing a split of a
// 1st Leaf of a partial groupby subtree.
// -----------------------------------------------------------------------
GroupByTernarySplitRule::~GroupByTernarySplitRule() {}

// topMatch() examines the Context.
NABoolean GroupByTernarySplitRule::isContextSensitive() const
{
  // WaveFix Begin
  // This rule is context insensitive when the WaveFix is On
  if (QueryAnalysis::Instance() &&
      QueryAnalysis::Instance()->dontSurfTheWave())
    return FALSE;
  // WaveFix End

  return TRUE;
}

NABoolean GroupByTernarySplitRule::topMatch(RelExpr * expr,
                                            Context * context)
{
  // WaveFix Begin
  // This is part of the fix for the count(*) wave
  // This rule is disabled by default, except if there
  // is a scalar aggregate query on a single multi-partition table, a query
  // like Select count(*) from fact;
  // In such a case we would like to use this rule to
  // get a layer of esps, this causes the plan to fixup
  // in parallel avoiding the serial fixup if the plan is
  // just the master executor on top of dp2. The serial
  // fixup causes the query to execute in wave pattern, since
  // each dp2 is fixed up and then starts execution. Due to serial
  // fixup a dp2 is fixed up, and then we move to the next
  // dp2 causing the wave pattern.
  if (!(QueryAnalysis::Instance() &&
        QueryAnalysis::Instance()->dontSurfTheWave()))
  {
    return FALSE; // $$$ pending debugging
  }
  // WaveFix End
  // Old Code Begin
  // return FALSE; // $$$ pending debugging
  // Old Code End


#pragma nowarn(203)   // warning elimination
  if (NOT Rule::topMatch(expr,context)) // MUST be a GroupByAgg
#pragma warn(203)  // warning elimination
    return FALSE;

  if (expr->getOperatorType() == REL_SHORTCUT_GROUPBY)
    return FALSE;

  // make sure we are looking at a group by node
  CMPASSERT(expr->getOperatorType() == REL_GROUPBY);

  GroupByAgg *bef = (GroupByAgg *) expr;

  // WaveFix Begin
  if (!(QueryAnalysis::Instance() &&
        QueryAnalysis::Instance()->dontSurfTheWave()))
  {
  // WaveFix End

    // Note this IF is not tested because of the "return FALSE"
    // at the beginning of the method.
    // If the location is in DP2, the groupBy is in a CS, and
    // push-down is requested, do not allow split.
    if (context AND
        context->getReqdPhysicalProperty()->executeInDP2() AND
        CURRSTMT_OPTDEFAULTS->pushDownDP2Requested() AND
        bef->isinBlockStmt()
       )
       return FALSE;

  // WaveFix Begin
  }
  // WaveFix End

  if (NOT bef->isAPartialGroupByLeaf1()) // MUST be a partial groupby leaf
    return FALSE;

  // ---------------------------------------------------------------------
  // Examine the aggregate functions. If any one of them cannot be
  // evaluated in stages, such as a partial aggregation followed
  // by finalization, then do not split this GroupByAgg.
  // ---------------------------------------------------------------------
  if (NOT bef->aggregateEvaluationCanBeStaged())
    return FALSE;

  // WaveFix Begin
  if (QueryAnalysis::Instance() &&
      QueryAnalysis::Instance()->dontSurfTheWave())
  {
    return TRUE;
  }
  // WaveFix End

  // ---------------------------------------------------------------------
  // Perform a ternary split for a partial groupby only when the required
  // physical property permits a degree of parallelism greater than
  // one or requires more than one partitions.
  // ---------------------------------------------------------------------
  if (context AND context->getReqdPhysicalProperty())
    {
      const ReqdPhysicalProperty* const rppForMe =
                                     context->getReqdPhysicalProperty();

      if ( (rppForMe->getCountOfAvailableCPUs() == 1) OR
           (rppForMe->getPartitioningRequirement() AND
            (rppForMe->getPartitioningRequirement()->
              getCountOfPartitions() == 1))  OR
           rppForMe->executeInDP2() )
        return FALSE;
    }
  else
    return FALSE;

 return TRUE;

} // GroupByTernarySplitRule::topMatch()

NABoolean GroupByTernarySplitRule::canMatchPattern(
     const RelExpr * pattern) const
{
  // consider both the substitute (a map value ids) and its child
  // as possible output patterns
  return (getSubstitute()->getOperator().match(pattern->getOperator()) OR
    getSubstitute()->child(0)->getOperator().match(pattern->getOperator()));
}

// -----------------------------------------------------------------------
// Aggregate Distinct Elimination Rule
//
// -----------------------------------------------------------------------
AggrDistinctEliminationRule::~AggrDistinctEliminationRule() {}

NABoolean AggrDistinctEliminationRule::topMatch(RelExpr * expr,
                                                Context * context)
{
  if (NOT Rule::topMatch(expr,context))
    return FALSE;

  if (expr->getOperatorType() == REL_SHORTCUT_GROUPBY)
    return FALSE;

  // make sure we are looking at a group by node
  CMPASSERT(expr->getOperatorType() == REL_GROUPBY);

  GroupByAgg *bef = (GroupByAgg *) expr;

  // ---------------------------------------------------------------------
  // See description above or in RelGrby.h.
  // ---------------------------------------------------------------------
  if (NOT bef->isNotAPartialGroupBy())
    return FALSE;

  ValueIdSet distinctColumns;              // columns (exprs) referenced
                                           // in non-redundant DISTINCT
                                           // aggregates
  CollIndex numNonMultiDistinctAggs = 0;

  const ValueIdSet &aggrs = bef->aggregateExpr();
  for (ValueId x = aggrs.init(); aggrs.next(x); aggrs.advance(x))
    {
      Aggregate *agg = (Aggregate *) x.getItemExpr();

      CMPASSERT(x.getItemExpr()->isAnAggregate());

      if (agg->isDistinct())
        {
          ValueIdSet uniqueSet = bef->groupExpr();
          uniqueSet += agg->getDistinctValueId();
          if (NOT bef->child(0).getGroupAttr()->isUnique(uniqueSet)) {
            distinctColumns += agg->getDistinctValueId();
            if((agg->getDistinctValueId() != agg->child(0)) ||
               (agg->getArity() > 1)) {
              numNonMultiDistinctAggs++;
            }
          }
        }
    }

  ULng32 dc = distinctColumns.entries();

  if (dc == 0)
    {
      // Do not fire this rule if there are no aggregate distincts.
      // Fire the GroupBySplitRule instead.
      return FALSE;
    }
  else if (dc > 1)
    {
      // This rule can't handle multiple combination of DISTINCTs, since
      // it needs to perform duplicate elimination, which can't be done
      // for different column lists at the same time. A future solution
      // will either have to make "dc" queries out of this, use the
      // transpose operator, or will be an implementation of multiple
      // distincts in the executor. The latter is the reason why we kept
      // this an optimizer rule and didn't apply it in the normalizer.
      // Indicate the real cause for the error here. The optimizer
      // will not generate an execution plan which will result in
      // an additional error after the end of the optimization phase.

      // Can handle multiple distinct values for most cases
      //
      if(numNonMultiDistinctAggs == 0) {
        return TRUE;
      }

      *CmpCommon::diags() << DgSqlCode(-6001);
      return FALSE;
    }

  return TRUE;

} // AggrDistinctEliminationRule::topMatch()

RelExpr * AggrDistinctEliminationRule::nextSubstitute(RelExpr * before,
                                                      Context * /*context*/,
                                                      RuleSubstituteMemory * & memory)
{
  GroupByAgg *bef = (GroupByAgg *) before;
  MapValueIds *topMap;
  GroupByAgg *upperGB;
  GroupByAgg *lowerGB;
  const ValueIdSet &aggrs = bef->aggregateExpr();
  ValueIdSet toBeMadeDistinct;
  ValueIdSet distinctAggregates;
  ValueIdList nonDistinctAggregates;
  ValueIdList origNonDistinctAggregates;

  // Create the default substitute, two groupbys stacked on each other
  // with a MapValueIds on the very top
  topMap  = (MapValueIds *) Rule::nextSubstitute(before,NULL,memory);
  upperGB = (GroupByAgg *) topMap->child(0).getPtr();
  lowerGB = (GroupByAgg *) upperGB->child(0).getPtr();

  // Clear the upper aggregate expressions, they will be rewritten.
  upperGB->aggregateExpr().clear();

  // rewrite the DISTINCT aggregate functions by eliminating the DISTINCT
  // - For each DISTINCT aggregate in the original list, add a copy
  //   of that function to the result, but without the DISTINCT.
  ValueId x = aggrs.init();
  for (; aggrs.next(x); aggrs.advance(x))
    {
      Aggregate *a = (Aggregate *) x.getItemExpr();

      NABoolean reallyDistinct = FALSE;
      if (a->isDistinct())
        {
          ValueIdSet uniqueSet = bef->groupExpr();
          uniqueSet += a->getDistinctValueId();
          if (NOT bef->child(0).getGroupAttr()->isUnique(uniqueSet))
            reallyDistinct = TRUE;
        }

      if (reallyDistinct)
        {
          // move a copy of this aggregate to the top groupby
          // and remove the DISTINCT
          Aggregate *b = (Aggregate *)
            a->copyTopNode(0, CmpCommon::statementHeap());
          b->setDistinct(FALSE);
          b->child(0) = a->child(0);
          b->synthTypeAndValueId();

          // collect all those arguments of DISTINCT groupbys that
          // really need to be made distinct, and ignore those that
          // already are distinct
          toBeMadeDistinct += a->getDistinctValueId();

          distinctAggregates += x;

          upperGB->aggregateExpr() += b->getValueId();
          // indicate that the result that is supposed to be stored in
          // x is now delivered through the rewritten aggregate function

          topMap->addMapEntry(x,b->getValueId());
          topMap->addValueForVEGRewrite(x);
        }
      else
        {
          nonDistinctAggregates.insert(x);
          origNonDistinctAggregates.insert(x);
        }
    }

  CollIndex distValues = toBeMadeDistinct.entries();

  // Check for special case of multiple distinct aggrs
  //
  Transpose *xPose = NULL;
  ValueIdMap distinctMap;
  if(distValues > 1) {

    xPose = new (CmpCommon::statementHeap()) Transpose();

    xPose->child(0) = lowerGB->child(0);
    lowerGB->child(0) = xPose;

    xPose->setTransUnionVectorSize(2);
    xPose->transUnionVector() = new (CmpCommon::statementHeap())
      ValueIdList[xPose->transUnionVectorSize()];

    CollIndex distNum = 0;
    for (x = upperGB->aggregateExpr().init();
         upperGB->aggregateExpr().next(x);
         upperGB->aggregateExpr().advance(x))
      {
        Aggregate *a = (Aggregate *) x.getItemExpr();

        // For each distinct value, create a Transpose set and
        // add a mapping from the result of the transpose to the
        // original distinct value.
        //
        if(!(distinctMap.getTopValues().contains(a->child(0)))) {

          ValueIdList transVals;
          ItemExpr *nullVal = new (CmpCommon::statementHeap()) ConstValue();

          NAType *nullType =
            a->child(0)->getValueId().getType().newCopy(CmpCommon::statementHeap());
	  nullType->setNullable(TRUE);

          nullVal = new (CmpCommon::statementHeap())
            Cast(nullVal, nullType);

          nullVal->synthTypeAndValueId();

          for(CollIndex i = 0; i < distValues; i++) {
            if(distNum == i)
              transVals.insert(a->child(0));
            else
              transVals.insert(nullVal->getValueId());
          }

          ValueIdUnion *valVidu = new(CmpCommon::statementHeap())
            ValueIdUnion(transVals, NULL_VALUE_ID);

          valVidu->synthTypeAndValueId();

          xPose->transUnionVector()[1].insert(valVidu->getValueId());
          distinctMap.addMapEntry(a->child(0), valVidu->getValueId());

          distNum++;
        }

        ValueId newVal;
        distinctMap.mapValueIdDown(a->child(0), newVal);

        a->child(0) = newVal;

	// Redrive type of aggregate since child is now nullable.
        a->synthTypeAndValueId(TRUE);
      }

    CMPASSERT(xPose && (distNum == distValues));

    toBeMadeDistinct = distinctMap.getBottomValues();

    for (CollIndex n = 0; n < nonDistinctAggregates.entries(); n++)
    {
        x = nonDistinctAggregates[n];

        Aggregate *a = (Aggregate *) x.getItemExpr();

        // make a copy of this aggregate
        // we will be changing it later

        Aggregate *b = (Aggregate *)
          a->copyTopNode(0, CmpCommon::statementHeap());
        b->child(0) = a->child(0);
        b->synthTypeAndValueId();

        nonDistinctAggregates[n] = b->getValueId();


        ValueIdList transVals;
        ItemExpr *nullVal = new (CmpCommon::statementHeap()) ConstValue();

        NAType *nullType =
          b->child(0)->getValueId().getType().newCopy(CmpCommon::statementHeap());
        nullType->setNullable(TRUE);

        nullVal = new (CmpCommon::statementHeap())
          Cast(nullVal, nullType);

        nullVal->synthTypeAndValueId();

        for(CollIndex i = 0; i < distValues; i++) {
          if(i == 0)
            transVals.insert(b->child(0));
          else
            transVals.insert(nullVal->getValueId());
        }

        ValueIdUnion *valVidu = new(CmpCommon::statementHeap())
          ValueIdUnion(transVals, NULL_VALUE_ID);

        valVidu->synthTypeAndValueId();

        xPose->transUnionVector()[1].insert(valVidu->getValueId());

        if((a->getOperatorType() == ITM_COUNT) &&
           !(b->child(0)->getValueId().getType().supportsSQLnullLogical())) {
          b->setOperatorType(ITM_COUNT_NONULL);
        }
        b->child(0) = valVidu->getValueId();

        // Redrive type of aggregate since child is now nullable.
        //
        b->synthTypeAndValueId(TRUE);

        distNum++;
    }

    CMPASSERT(xPose &&
              (distNum == distValues + nonDistinctAggregates.entries()));

  }

  // The query contains "real" DISTINCT aggregates that require another
  // groupby operator (lowerGB).

  // - For all non-distinct aggregate functions, rewrite them into two,
  //   one for the upper node and one for the lower node.
  ValueIdList lowerValueIds;
  ValueIdList upperValueIds;
    for (CollIndex n = 0; n < nonDistinctAggregates.entries(); n++)
    {
      x = nonDistinctAggregates[n];

      Aggregate *a = (Aggregate *) x.getItemExpr();

      // Split the aggregate up in two parts, one to be evaluated
      // in the lower, and the other to be evaluated in the upper
      // groupby. Also, replace the output value id of the node
      // with the rewritten function.
      ValueId newAggResult =
          a->rewriteForStagedEvaluation
                (lowerValueIds,upperValueIds, TRUE)->getValueId();

      x = origNonDistinctAggregates[n];
      topMap->addMapEntry(x,newAggResult);
    }



  // add the lower and upper valueIds to the appropriate sets
  Int32 valusIdIdx = 0;
  for (;
       valusIdIdx < (Int32)lowerValueIds.entries(); valusIdIdx++)
    lowerGB->aggregateExpr() += lowerValueIds[valusIdIdx];
  for (valusIdIdx = 0; valusIdIdx < (Int32)upperValueIds.entries(); valusIdIdx++)
    upperGB->aggregateExpr() += upperValueIds[valusIdIdx];

  // Set aggDistElimRuleCreates flag to TRUE for both new GroupByAgg exprs.
  // When this flag is set TRUE, "MVQR GroupBy Rewrite Rule" will not be
  // fired.
  lowerGB->setAggDistElimRuleCreates(TRUE);
  upperGB->setAggDistElimRuleCreates(TRUE);

  // Set the grouping expression for the lower groupby: it has the
  // original grouping expressions plus those expressions that are
  // used in the DISTINCT aggregates. This means that we are effectively
  // doing the duplicate elimination that the DISTINCT in the original
  // aggregate function wanted.
  lowerGB->groupExpr() = upperGB->groupExpr();

  // may have to expand multi-valued item lists in the distinct columns $$$$
  lowerGB->groupExpr() += toBeMadeDistinct;

  // Map the HAVING predicate using the map from the newly allocated
  // MapValueIds node. This used to be done by attaching the HAVING
  // predicate to the mvi node and letting pushdown do the mapping,
  // then asserting that the predicate did, in fact, get pushed down.
  // The problem with this is that if the predicate only has outer
  // references and does not reference any columns that are output
  // by the child of the mvi then the predicate will not be pushed
  // down (see VegPredicate::isCovered). An example predicate is
  // t1.vch7 = 'b', where t1.vch7 is an outer reference.
  ValueIdSet selPredsRewritten;
  ValueIdSet selPreds = upperGB->selectionPred();
  topMap->getMap().rewriteValueIdSetDown(selPreds,selPredsRewritten);
  upperGB->selectionPred() = selPredsRewritten;

  // Prime the group attributes for the newly introduced nodes
  topMap->allocateAndPrimeGroupAttributes();

  topMap->pushdownCoveredExpr
    (topMap->getGroupAttr()->getCharacteristicOutputs(),
     topMap->getGroupAttr()->getCharacteristicInputs(),
     topMap->selectionPred());
  // Refine the Characteristic Inputs and Outputs of the lower GroupBy.
  // and refine the Characteristic Inputs and Outputs of the lower
  // GroupBy.
  // Pushdown HAVING predicates from upperGB to lowerGB, if possible.
  // Refine the Characteristic Inputs and Outputs of lowerGB.
  upperGB->pushdownCoveredExpr
    (upperGB->getGroupAttr()->getCharacteristicOutputs(),
     upperGB->getGroupAttr()->getCharacteristicInputs(),
     upperGB->selectionPred());

  // pushdownCoveredExpr() is not called on the lower GroupBy because
  // its child should be a CutOp; it is not legal to pushdown a
  // predicate and modify the Group Attributes of the Cascades GROUP
  // that it belongs to.
  if(xPose) {
    lowerGB->pushdownCoveredExpr
      (lowerGB->getGroupAttr()->getCharacteristicOutputs(),
       lowerGB->getGroupAttr()->getCharacteristicInputs(),
       lowerGB->selectionPred());
  }

  topMap->child(0)->synthLogProp();

  return topMap;
}

NABoolean AggrDistinctEliminationRule::canMatchPattern (
     const RelExpr * pattern) const
{
  // consider the both the substitute (a map value ids) and its child
  // as possible output patterns
  return (getSubstitute()->getOperator().match(pattern->getOperator()) OR
    getSubstitute()->child(0)->getOperator().match(pattern->getOperator()));
}

// -----------------------------------------------------------------------
// GroupBySplitRule
// -----------------------------------------------------------------------
GroupBySplitRule::~GroupBySplitRule() {}

NABoolean GroupBySplitRule::topMatch(RelExpr * expr,
                                     Context * context)
{
  // check if this rule has been disabled via RuleGuidanceCQD
  // the CQD is COMP_INT_77 and it represents a bitmap
  // below we check if the bit # 5 is ON
  if(CURRSTMT_OPTDEFAULTS->isRuleDisabled(5))
    return FALSE;

  if (NOT Rule::topMatch(expr,context))
    return FALSE;

  if (expr->getOperatorType() == REL_SHORTCUT_GROUPBY)
    return FALSE;

  // make sure we are looking at a group by node
  CMPASSERT(expr->getOperatorType() == REL_GROUPBY);

  GroupByAgg *bef = (GroupByAgg *) expr;

  // ---------------------------------------------------------------------
  // See description above or in RelGrby.h.
  // ---------------------------------------------------------------------
  if (NOT bef->isNotAPartialGroupBy())
    return FALSE;

  // ---------------------------------------------------------------------
  // Examine the aggregate functions. If any one of them cannot be
  // evaluated in stages, such as a partial aggregation followed
  // by finalization, then do not split this GroupByAgg.
  // ---------------------------------------------------------------------
  if (NOT bef->aggregateEvaluationCanBeStaged())
    return FALSE;

  ValueIdSet distinctColumns;              // columns (exprs) referenced
                                           // in non-redundant DISTINCT
                                           // aggregates
  const ValueIdSet &aggrs = bef->aggregateExpr();

  for (ValueId x = aggrs.init(); aggrs.next(x); aggrs.advance(x))
    {
      Aggregate *agg = (Aggregate *) x.getItemExpr();

      CMPASSERT(x.getItemExpr()->isAnAggregate());

      if (agg->isDistinct())
        {
          ValueIdSet uniqueSet = bef->groupExpr();
          uniqueSet += agg->getDistinctValueId();
          if (NOT bef->child(0).getGroupAttr()->isUnique(uniqueSet))
            distinctColumns += agg->getDistinctValueId();
        }
    }

#pragma nowarn(1506)   // warning elimination
  Lng32 dc = distinctColumns.entries();
#pragma warn(1506)  // warning elimination

  if (dc > 0)
    {
      // If there exists an aggregate distinct, fire the
      // AggrDistinctEliminationRule instead.

      return FALSE;
    }

  // Do not split the group by if it can be eliminated
  if (NOT bef->groupExpr().isEmpty())
    return NOT (bef->child(0).getGroupAttr()->isUnique(bef->groupExpr()));

  return TRUE;

} // GroupBySplitRule::topMatch()

RelExpr * GroupBySplitRule::nextSubstitute(RelExpr * before,
                                           Context * /*context*/,
                                           RuleSubstituteMemory * & memory)
{
  GroupByAgg *bef = (GroupByAgg *) before;
  MapValueIds *topMap;
  GroupByAgg *upperGB;
  GroupByAgg *lowerGB;
  const ValueIdSet &aggrs = bef->aggregateExpr();
  ValueIdSet toBeMadeDistinct;
  ValueIdSet nonDistinctAggregates;

  // Create the default substitute, two groupbys stacked on each other
  // with a MapValueIds on the very top
  topMap  = (MapValueIds *) Rule::nextSubstitute(before,NULL,memory);
  upperGB = (GroupByAgg *) topMap->child(0).getPtr();
  lowerGB = (GroupByAgg *) upperGB->child(0).getPtr();

  // Mark the two new GroupBy operators in the substitute as
  // split GroupBy operators (see comments in RelGrby.h). This
  // is done in order to prevent further splitting by the application
  // of the GroupBySplitRule.
  assert(bef->isNotAPartialGroupBy() OR bef->isAPartialGroupByLeaf1());
  if (bef->isNotAPartialGroupBy())
    {
      upperGB->markAsPartialGroupByRoot();
      lowerGB->markAsPartialGroupByLeaf1();
    }
  else
    {
      upperGB->markAsPartialGroupByNonLeaf();
      lowerGB->markAsPartialGroupByLeaf2();
    }

  // The grouping and aggregate expressions have already been
  // assigned to upperGB. This is because the pattern
  // for this rule contains a WildCardOp. The WildCardOp provides the
  // GroupByAgg pointed to by bef to copyTopNode as the instance
  // to be copied.

  // Clear the upper aggregate expressions, they will be rewritten.
  upperGB->aggregateExpr().clear();

  // rewrite the DISTINCT aggregate functions by eliminating the DISTINCT
  // - For each DISTINCT aggregate in the original list, add a copy
  //   of that function to the result, but without the DISTINCT.
  ValueId x = aggrs.init();
  for (; aggrs.next(x); aggrs.advance(x))
    {
      Aggregate *a = (Aggregate *) x.getItemExpr();

      NABoolean reallyDistinct = FALSE;
      if (a->isDistinct())
        {
          ValueIdSet uniqueSet = bef->groupExpr();
          uniqueSet += a->getDistinctValueId();
          if (NOT bef->child(0).getGroupAttr()->isUnique(uniqueSet))
            reallyDistinct = TRUE;
        }

      if (reallyDistinct)
        {
          // move a copy of this aggregate to the top groupby
          // and remove the DISTINCT
          Aggregate *b = (Aggregate *)
            a->copyTopNode(0, CmpCommon::statementHeap());
          b->setDistinct(FALSE);
          b->child(0) = a->child(0);
          b->synthTypeAndValueId();

          // collect all those arguments of DISTINCT groupbys that
          // really need to be made distinct, and ignore those that
          // already are distinct
          toBeMadeDistinct += a->getDistinctValueId();

          upperGB->aggregateExpr() += b->getValueId();
          // indicate that the result that is supposed to be stored in
          // x is now delivered through the rewritten aggregate function

          topMap->addMapEntry(x,b->getValueId());
        }
      else
        {
          nonDistinctAggregates += x;
        }
    }

  // The query contains "real" DISTINCT aggregates that require another
  // groupby operator (lowerGB).

  // - For all non-distinct aggregate functions, rewrite them into two,
  //   one for the upper node and one for the lower node.
  ValueIdList lowerValueIds;
  ValueIdList upperValueIds;
  for (x = nonDistinctAggregates.init();
       nonDistinctAggregates.next(x);
       nonDistinctAggregates.advance(x))
    {
      Aggregate *a = (Aggregate *) x.getItemExpr();

      // Split the aggregate up in two parts, one to be evaluated
      // in the lower, and the other to be evaluated in the upper
      // groupby. Also, replace the output value id of the node
      // with the rewritten function.
      ValueId newAggResult =
          a->rewriteForStagedEvaluation
                (lowerValueIds,upperValueIds, TRUE)->getValueId();

      topMap->addMapEntry(x,newAggResult);
    }

  // add the lower and upper valueIds to the appropriate sets
  Int32 valusIdIdx = 0;
  for (;
       valusIdIdx < (Int32)lowerValueIds.entries(); valusIdIdx++)
    lowerGB->aggregateExpr() += lowerValueIds[valusIdIdx];
  for (valusIdIdx = 0; valusIdIdx < (Int32)upperValueIds.entries(); valusIdIdx++)
    upperGB->aggregateExpr() += upperValueIds[valusIdIdx];

  // Set the grouping expression for the lower groupby: it has the
  // original grouping expressions plus those expressions that are
  // used in the DISTINCT aggregates. This means that we are effectively
  // doing the duplicate elimination that the DISTINCT in the original
  // aggregate function wanted.
  lowerGB->groupExpr() = upperGB->groupExpr();

  // may have to expand multi-valued item lists in the distinct columns $$$$
  lowerGB->groupExpr() += toBeMadeDistinct;

  // Map the HAVING predicate using the map from the newly allocated
  // MapValueIds node. This used to be done by attaching the HAVING
  // predicate to the mvi node and letting pushdown do the mapping,
  // then asserting that the predicate did, in fact, get pushed down.
  // The problem with this is that if the predicate only has outer
  // references and does not reference any columns that are output
  // by the child of the mvi then the predicate will not be pushed
  // down (see VegPredicate::isCovered). An example predicate is
  // t1.vch7 = 'b', where t1.vch7 is an outer reference.
  ValueIdSet selPredsRewritten;
  ValueIdSet selPreds = upperGB->selectionPred();
  topMap->getMap().rewriteValueIdSetDown(selPreds,selPredsRewritten);
  upperGB->selectionPred() = selPredsRewritten;

  // Prime the group attributes for the newly introduced nodes
  topMap->allocateAndPrimeGroupAttributes();

  topMap->pushdownCoveredExpr
    (topMap->getGroupAttr()->getCharacteristicOutputs(),
     topMap->getGroupAttr()->getCharacteristicInputs(),
     topMap->selectionPred());
  // Refine the Characteristic Inputs and Outputs of the lower GroupBy.
  // and refine the Characteristic Inputs and Outputs of the lower
  // GroupBy.
  // Pushdown HAVING predicates from upperGB to lowerGB, if possible.
  // Refine the Characteristic Inputs and Outputs of lowerGB.
  upperGB->pushdownCoveredExpr
    (upperGB->getGroupAttr()->getCharacteristicOutputs(),
     upperGB->getGroupAttr()->getCharacteristicInputs(),
     upperGB->selectionPred());

  // pushdownCoveredExpr() is not called on the lower GroupBy because
  // its child should be a CutOp; it is not legal to pushdown a
  // predicate and modify the Group Attributes of the Cascades GROUP
  // that it belongs to.
  topMap->child(0)->synthLogProp();

  return topMap;
}

Int32 GroupBySplitRule::promiseForOptimization(RelExpr *,
                                               Guidance *,
                                               Context *)
{
  return DefaultTransRulePromise;
}

NABoolean GroupBySplitRule::canMatchPattern (const RelExpr * pattern) const
{
  // consider the both the substitute (a map value ids) and its child
  // as possible output patterns
  return (getSubstitute()->getOperator().match(pattern->getOperator()) OR
    getSubstitute()->child(0)->getOperator().match(pattern->getOperator()));
}

// -----------------------------------------------------------------------
// methods for class GroupByOnJoinRule
// -----------------------------------------------------------------------
GroupByOnJoinRule::~GroupByOnJoinRule() {}

NABoolean GroupByOnJoinRule::topMatch (RelExpr * expr,
                                       Context * /*context*/)
{
  // check if this rule has been disabled via RuleGuidanceCQD
  // the CQD is COMP_INT_77 and it represents a bitmap
  // below we check if the bit # 3 is ON
  if(CURRSTMT_OPTDEFAULTS->isRuleDisabled(3))
    return FALSE;

  if (NOT Rule::topMatch(expr,NULL))
    return FALSE;

  if (expr->getOperatorType() == REL_SHORTCUT_GROUPBY)
    return FALSE;

  // make sure we are looking at a group by node
  CMPASSERT(expr->getOperatorType() == REL_GROUPBY);

  GroupByAgg *bef = (GroupByAgg *) expr;

  // ---------------------------------------------------------------------
  // If this GroupByAgg was created by the GroupBySplitRule, no
  // further transformation rules should be applied to it. The
  // GroupBySplitRule replaces GB(CutOp) with a GB(GB(CutOp)).
  // Our intention is to apply the relevant transformation rules
  // to the original pattern but not to its substitute so that each
  // GroupByAgg in the substitute is subjected to implementation
  // rules only.
  // ---------------------------------------------------------------------
  if (NOT bef->isNotAPartialGroupBy())
    return FALSE;

  // Do not split the group by if it can be eliminated
  if (NOT bef->groupExpr().isEmpty())
    {
      // do not eliminate group by if rollup is being done
      if (bef->isRollup())
        return FALSE;
      
      return NOT (bef->child(0).getGroupAttr()->isUnique(bef->groupExpr()));
    }

  // the functional dependencies shown below won't hold if this is an
  // aggregate (ok, there are some sick examples where they do, but
  // those aren't important for a commercial DBMS)
  return NOT bef->groupExpr().isEmpty();
}

RelExpr * GroupByOnJoinRule::nextSubstitute(RelExpr * before,
                                            Context * /*context*/,
                                            RuleSubstituteMemory * & /*memory*/)
{
  // ---------------------------------------------------------------------
  // The idea is to transform a query with a groupby on top of a join
  // into a query with the groupby below the join:
  //
  //            t1.grcol,t2.grcol            pred(t1.jcol,t2.jcol)
  //      grby  t1.aggr,t2.aggr                                   join
  //        |                                                     /  \
  //        |   pred(t1.jcol,t2.jcol)         t1.grcol,t1.jcol   /    \
  //      join                        ---->            t1.aggr grby   t2
  //      /  \                                                  |
  //     /    \                                                 |
  //    t1    t2                                               t1
  //
  // There are three conditions for firing the rule:
  //
  //    (1)  The grouping expressions can be separated into a part
  //         t1.grcol that is covered by t1 and a part t2.grcol that
  //         is covered by t2.
  //
  //    (2)  The aggregate functions only reference values from t1.
  //
  //    (3)  In the result of the join, the groupby columns
  //         (t1.grcol,t2.grcol) contain or cover the join predicate
  //         values from t1, and a candidate-key of t2.
  //         Expressed as functional dependency that looks like
  //
  //            (t1.grcol,t2.grcol)   ---->  (t1.jcol,t2.key).
  //
  // Further info can be found in a paper published by Weipeng
  // Paul Yan and Per-Ake Larson in the proceedings of the 10th
  // IEEE Data Engineering Conference, Houston, TX, 1994, pp. 89-100.
  // http://www.informatik.uni-trier.de/~ley/db/conf/icde/YanL94.html
  //
  // Besides the substitute shown above, the following substitutes are
  // also generated in some cases:
  //
  //        join
  //        /  \          if the aggregate functions only reference
  //       /    \         values from t2 and if the join commutativity
  //      t1   grby       cannot be applied on the join between t1 and t2
  //             |
  //             |        (needed if we restrict the commutativity rule)
  //            t2
  //
  // ---------------------------------------------------------------------

  CMPASSERT(before->getOperatorType() == REL_GROUPBY);

  GroupByAgg *oldGB = (GroupByAgg *) before;
  Join *oldJoin = (Join *) before->child(0).getPtr();

  // Check to see whether we should apply any more transformation rules on this
  // join child.
  if (oldJoin->isTransformComplete())
    return NULL;

  const ValueIdSet &aggrs = oldGB->aggregateExpr();
  ValueIdSet grcol(oldGB->groupExpr());
  NABoolean reverseT1T2 = FALSE;

  // to be used for coverTest
  ValueIdSet referencedInputs;
  ValueIdSet coveredExpr;
  ValueIdSet coveredSubExpr;

  // ---------------------------------------------------------------------
  // check condition (1), can grouping cols be separated?
  // ---------------------------------------------------------------------

  ValueIdSet t1grcol;
  ValueIdSet t2grcol;

  referencedInputs.clear();
  if (NOT grcol.isEmpty())
    {
      // which grouping columns are covered by t1?
      oldJoin->child(0).getGroupAttr()->coverTest(
           grcol,
           oldGB->getGroupAttr()->getCharacteristicInputs(),
           t1grcol,
           referencedInputs);

      // if not all of them are covered by t1, check which
      // ones are covered by t2
      if (t1grcol != grcol)
        {
          referencedInputs.clear();
          oldJoin->child(1).getGroupAttr()->coverTest(
               grcol,
               oldGB->getGroupAttr()->getCharacteristicInputs(),
               t2grcol,
               referencedInputs);

          // all remaining groupby columns have to be covered by t2!!
          // note that we ignored any covered subexpressions, we are
          // only interested in groupby expressions that are covered
          // as a whole
          ValueIdSet rest(grcol);

          rest -= t1grcol;
          rest -= t2grcol;
          if (NOT rest.isEmpty())
            return NULL;
        }
    }

  // ---------------------------------------------------------------------
  // check condition (2), do the aggregates only reference one table?
  // ---------------------------------------------------------------------

  ValueIdSet aggrInputs;

  oldGB->getValuesRequiredForEvaluatingAggregate(aggrInputs);

  // are the children covered by the left table, t1?
  coveredExpr.clear();
  referencedInputs.clear();
  oldJoin->child(0).getGroupAttr()->coverTest(
       aggrInputs,
       oldGB->getGroupAttr()->getCharacteristicInputs(),
       coveredExpr,
       referencedInputs);

  if (coveredExpr != aggrInputs)
    {
      // no, children aren't covered by t1, try the right child, t2
      coveredExpr.clear();
      referencedInputs.clear();
      oldJoin->child(1).getGroupAttr()->coverTest(
           aggrInputs,
           oldGB->getGroupAttr()->getCharacteristicInputs(),
           coveredExpr,
           referencedInputs);
      if (coveredExpr == aggrInputs)
        // All aggregates are covered by table t2, try it the reverse way.
        reverseT1T2 = TRUE;
      else
        // sorry, aggregates reference both input tables
        return NULL;
    }


  if (reverseT1T2)
    {
      // don't fire the rule with reversed roles if we can apply the
      // join commutativity rule on the join and the CQD is OFF
      if (oldJoin->getGroupAttr()->getNumJoinedTables() <= 2 &&
          CmpCommon::getDefault(GROUP_BY_PUSH_TO_BOTH_SIDES_OF_JOIN) != DF_ON)
        return NULL;

      // don't reverse the roles if the join isn't symmetric (such as a
      // left join, semi-join, or TSJ)
      if (oldJoin->getOperatorType() != REL_JOIN)
        return NULL;
    }

  // ---------------------------------------------------------------------
  // compute t1.jcol, t2.jcol
  // ---------------------------------------------------------------------

  // The variable joinPreds should contain both selection predicate
  // and the join predicate. -Genesis Case 10-981103-1385
  ValueIdSet joinPreds(oldJoin->selectionPred());
  joinPreds += oldJoin->joinPred();

  ValueIdSet t1jcol;
  ValueIdSet t2jcol;

  if (NOT (oldJoin->isTSJ()))
  {

    // Special handling of VEGPredicates:a VEGPredicate in the join predicate
    // means that a VEGReference should be covered by both input tables.
    // Convert all VEGPredicates into VEGReferences.
    for (ValueId x = joinPreds.init(); joinPreds.next(x); joinPreds.advance(x))
      {
        if (x.getItemExpr()->getOperatorType() == ITM_VEG_PREDICATE)
          {
            VEGPredicate *v = (VEGPredicate *) x.getItemExpr();

            joinPreds -= x;
            joinPreds += v->getVEG()->getVEGReference()->getValueId();
          }
      }

    if (NOT joinPreds.isEmpty())
      {
        // Which grouping columns are covered by t1 and t2?
        referencedInputs.clear();
        coveredSubExpr.clear();
        oldJoin->child(0).getGroupAttr()->coverTest(
           joinPreds,
           oldGB->getGroupAttr()->getCharacteristicInputs(),
           t1jcol,
           referencedInputs,
           &coveredSubExpr);
        t1jcol += coveredSubExpr;
        referencedInputs.clear();
        coveredSubExpr.clear();
        oldJoin->child(1).getGroupAttr()->coverTest(
           joinPreds,
           oldGB->getGroupAttr()->getCharacteristicInputs(),
           t2jcol,
           referencedInputs,
           &coveredSubExpr);
        t2jcol += coveredSubExpr;
      }
  }
  else
  {
    // For TSJs, calculate the t1jcol by subtracting the char. inputs of
    // the join from the char. inputs of t2.  The ones remaining are guaranteed
    // to be those input values coming from t1.

    // No need to compute t2jcol since we can't reverseT1T2 for a TSJ.

    t1jcol = oldJoin->child(1).getGroupAttr()->getCharacteristicInputs();
    t1jcol -= oldJoin->getGroupAttr()->getCharacteristicInputs();
  }

  // ---------------------------------------------------------------------
  // check condition (3), the functional dependencies
  // ---------------------------------------------------------------------

  // Is t1jcol covered by the grouping columns?
  // Initialize a group attributes object with the grouping columns as its
  // characteristic outputs. If t1jcol or t2jcol is covered by those empty
  // group attributes, it can be computed from grcol.
  GroupAttributes gra;
  const ValueIdSet *tjcol;

  if (reverseT1T2)
    tjcol = &t2jcol;
  else
    tjcol = &t1jcol;

  // add all columns that are functionally dependent on grcol,
  // this will increase the chance that we cover the join columns
  const ValueIdSet &constr = oldGB->getGroupAttr()->getConstraints();

  for (ValueId v = constr.init(); constr.next(v); constr.advance(v))
    {
      if (v.getItemExpr()->getOperatorType() == ITM_FUNC_DEPEND_CONSTRAINT)
	{
	  FuncDependencyConstraint *fdc =
	    (FuncDependencyConstraint *) v.getItemExpr();

	  if (grcol.contains(fdc->getDeterminingCols()))
	    grcol += fdc->getDependentCols();
	}
    }

  gra.addCharacteristicOutputs(grcol);

  coveredExpr.clear();
  referencedInputs.clear();
  gra.coverTest(*tjcol,
                oldGB->getGroupAttr()->getCharacteristicInputs(),
                coveredExpr,
                referencedInputs);

  if (coveredExpr != *tjcol)
    return NULL;

  // get to the constraints for t2 (check whether we reversed the
  // roles of t1 and t2)
  const GroupAttributes *cga;

  if (reverseT1T2)
    cga = oldJoin->child(0).getGroupAttr();
  else
    cga = oldJoin->child(1).getGroupAttr();

  // is t2.key functionally dependent on the grouping columns?
  if (NOT cga->isUnique(grcol))
    {
      // this simple test for uniqueness didn't work, try walking
      // through functional dependency constraints of the join
      // to see whether we can show a functional dependency
      // grcol --> t2.key.

      // NOTE: We also try to put the functional dependency together
      // from multiple constraints, e.g. (a,b,c) --> (d,e,f) could be
      // derived from the following two FD constraints:
      //    (a,b) --> d
      //    (a,b,c) --> (e,f)

      const ValueIdSet &cc =
	oldJoin->getGroupAttr()->getConstraints();
      ValueIdSet grcolPlusDependents(grcol);

      for (ValueId ccv = cc.init(); cc.next(ccv); cc.advance(ccv))
	{
	  if (ccv.getItemExpr()->getOperatorType() ==
	      ITM_FUNC_DEPEND_CONSTRAINT)
	    {
	      FuncDependencyConstraint *ccfd =
		(FuncDependencyConstraint *) ccv.getItemExpr();

	      if (grcolPlusDependents.contains(ccfd->getDeterminingCols()))
		{
		  grcolPlusDependents += ccfd->getDependentCols();
		}
	    }
	}

      // part of fix to allow GroupBySplitRule and GroupByOnJoinRule
      // to do eager aggregation on BP wellevent queries
      //   SELECT ..., AVG(p.PI_VALUE) AS AverageofPIValues, ...
      //   FROM Equipment e INNER JOIN PI_VALUE p
      //   ON	e.EXP_EP    = p.EXP_EP
      //   AND  e.EXP_DWGOM = p.EXP_DWGOM
      //   AND  e.Equipment_Location = p.PI_TAG
      //   WHERE ...
      //   GROUP BY p.PI_TAG, e.API, e.EXP_EP, e.EXP_DWGOM;
      // The immediate cause for mxcmp's inability to push groupby below
      // the join is cga->isUnique(grcolPlusDependents) returns FALSE
      // even though "grcol --> t2.key" is true for the BP query.
      // The root cause of the problem is the prevention of VEG formation
      // for the "e.Equipment_Location = p.PI_TAG" varchar join predicate.
      // We really want VEGs to form even for varchar equality predicates,
      // but, until that happens, we compensate for the missing VEG.
      // That is, in order to establish that
      //   (p.pi_tag, e.api, e.exp_ep, e.exp_dwgom) -->
      //   (e.exp_ep, e.exp_dwgom, e.equipment_location)
      // we introduce the missing VEG for the equijoin predicate
      //   "p.pi_tag = e.equipment_location"
      if (CmpCommon::getDefault(COMP_BOOL_177) == DF_OFF)
        joinPreds.introduceMissingVEGRefs(grcolPlusDependents);

      // To test grcol --> t2.key we want the left side of the
      // functional dependency to be grcol and all of its dependent
      // values, and we want some key or candidate key (a unique set
      // of columns) on the right. The actual test is done by checking
      // uniqueness of grcolPlusDependents.
      if (NOT cga->isUnique(grcolPlusDependents))
	{
	  // sorry, the rule is not applicable since we cannot prove that
	  // t2.key is functionally dependent on the grouping columns
	  return NULL;
	}
    }

  // ---------------------------------------------------------------------
  // conditions (1), (2), and (3) are met, now create the substitute
  // ---------------------------------------------------------------------

  Join        *newJoin  = (Join *)
    oldJoin->copyTopNode(0, CmpCommon::statementHeap());
  GroupByAgg  *newGB    = (GroupByAgg *)
    oldGB->copyTopNode(0, CmpCommon::statementHeap());
  
  newJoin->setGroupAttr(before->getGroupAttr());
  newGB->setGroupAttr(new (CmpCommon::statementHeap()) GroupAttributes());

  // Set groupByOnJoinRuleCreates flag to TRUE for new group by expr.
  // When this flag is set TRUE, "MVQR GroupBy Rewrite Rule" will not be
  // fired.
  newGB->setGroupByOnJoinRuleCreates(TRUE);

  // -- pruning based on potential -- begin
  // combinedPotential determines if a task on a expression is pruned
  // CURRSTMT_OPTDEFAULTS->pruneByOptLevel
  Int32 oldJoinCombinedPotential =
    oldJoin->getPotential() + oldJoin->getGroupAttr()->getPotential();
  Int32 originalGroupPotential = before->getGroupAttr()->getPotential();
  
  // newJoin should have the same combined potential as the oldJoin
  Int32 newJoinPotential = oldJoinCombinedPotential - originalGroupPotential; 
  newJoin->setPotential(newJoinPotential);
  
  // newGB should have the same combined potential as the new join, this is
  // because if tasks on one of them (i.e. newJoin, newGB) are pruned then
  // tasks on both of them should be pruned, otherwise the work done is useless
  // since the tasks will not contribute toward getting a full plan
  newGB->setPotential(newJoinPotential);
  newGB->getGroupAttr()->updatePotential(originalGroupPotential);
  // -- pruning based on potential -- end

  // move any HAVING predicates from the GB node to the join node
  newJoin->selectionPred() += newGB->getSelectionPred();
  newGB->selectionPred().clear();

  // put the substitute tree together

  ValueIdSet  constantValues ;
  if (reverseT1T2)
    {
      // reversed roles, produce join(cut(0),grby(cut(1)))
      newJoin->child(0) = oldJoin->child(0).getPtr();
      newJoin->child(1) = newGB;
      newGB->child(0) = oldJoin->child(1).getPtr();
      newGB->aggregateExpr() = aggrs;
      newGB->groupExpr() = t2grcol;
      t2jcol.getConstants(constantValues);
      t2jcol.remove(constantValues);
      newGB->groupExpr() += t2jcol;
    }
  else
    {
      // produce join(grby(cut(0)),cut(1))
      newJoin->child(0) = newGB;
      newJoin->child(1) = oldJoin->child(1).getPtr();
      newGB->child(0) = oldJoin->child(0).getPtr();
      newGB->aggregateExpr() = aggrs;
      newGB->groupExpr() = t1grcol;
      t1jcol.getConstants(constantValues);
      t1jcol.remove(constantValues);
      newGB->groupExpr() += t1jcol;
    }


  // ---------------------------------------------------------------------
  // Don't create a groupby node with no grouping columns. The definition
  // of such a node that this means computing an aggregate and always
  // returning a row doesn't work for this query transformation. We need
  // one row back if there is data in the table, otherwise we need 0 rows.
  // Add the predicate "count(*) > 0" and the aggregate "count(*)" to the
  // empty groupby node.
  // $$$$ re-introduce the REL_AGGREGATE id and change the generator to
  // return 0 rows for a groupby with an empty input table. Then delete
  // the code below.
  // ---------------------------------------------------------------------
  if (newGB->groupExpr().isEmpty())
    {
      ValueId valId;
      for (valId = newGB->aggregateExpr().init();
           newGB->aggregateExpr().next(valId);
           newGB->aggregateExpr().advance(valId))
        {
          // this aggregate has now become a scalar aggregate.
          // Re-type it since a scalar aggr has different attributes
          // than a non-scalar aggregate (scalar aggr are nullable).

          if ( (valId.getItemExpr()->getOperatorType() == ITM_MIN) ||
               (valId.getItemExpr()->getOperatorType() == ITM_MAX) ||
               (valId.getItemExpr()->getOperatorType() == ITM_SUM) )
            {
             // Need to set the flag inScalarGroupBy as this
             // aggregate may be re-synthesized; or a derived aggregate
             // may need to have this information. (case10-001227-0392)
             // This needs to done irrespective of whether we
             // re-synthesize the type now.

             Aggregate * aggr = (Aggregate *)valId.getItemExpr();
             aggr->setInScalarGroupBy();

             if (NOT valId.getType().supportsSQLnull())
               {
                aggr->synthTypeAndValueId(TRUE /*resynthesize*/);
               }
            }
        }

      Aggregate *dummyAgg = new (CmpCommon::statementHeap())
        Aggregate(ITM_COUNT,
                  new (CmpCommon::statementHeap()) SystemLiteral(1));
      BiRelat *pred = new (CmpCommon::statementHeap())
        BiRelat(ITM_GREATER,
                dummyAgg,
                new (CmpCommon::statementHeap()) SystemLiteral(0));

      pred->synthTypeAndValueId();
      newGB->aggregateExpr() += dummyAgg->getValueId();
      newGB->selectionPred() += pred->getValueId();
      // Add the ValueIds of the two constants generated here to
      // the Charcateristic Inputs of newGB.
      newGB->getGroupAttr()->addCharacteristicInputs
                                (dummyAgg->child(0)->getValueId());
      newGB->getGroupAttr()->addCharacteristicInputs
                                (pred->child(1)->getValueId());

    }

  // ---------------------------------------------------------------------
  // set up the group attributes and logical properties of the substitute
  // ---------------------------------------------------------------------
  newJoin->allocateAndPrimeGroupAttributes();

  // Refine the Characteristic Inputs and Outputs of the join
  newJoin->pushdownCoveredExpr
    (newJoin->getGroupAttr()->getCharacteristicOutputs(),
     newJoin->getGroupAttr()->getCharacteristicInputs(),
     newJoin->selectionPred());

  // pushdownCoveredExpr() is not called onthe GroupBy because
  // its child should be a CutOp; it is not legal to pushdown a
  // predicate and modify the Group Attributes of the Cascades GROUP
  // that it belongs to.

  // synthesize logical properties for the new GB node
  newGB->synthLogProp();
  newGB->getGroupAttr()->addToAvailableBtreeIndexes(
    newGB->child(0).getGroupAttr()->getAvailableBtreeIndexes());

  // Call synthLogProp on the result also so that we can call
  // findEquiJoinPredicates() with the new set of predicates
  newJoin->synthLogProp();

  // If  oldJoin resulted from a application of MJPrimeTableRule, directly
  // or indirectly, we set newJoin to be also from MJPrimeTableRule.
  if (oldJoin->isJoinFromPTRule())
    newJoin->setJoinFromPTRule();

  return newJoin;
}

// -----------------------------------------------------------------------
// Methods for class PartialGroupByOnTSJRule
// -----------------------------------------------------------------------
PartialGroupByOnTSJRule::~PartialGroupByOnTSJRule() {}

NABoolean PartialGroupByOnTSJRule::topMatch (RelExpr * expr,
                                             Context * /*context*/)
{
  // check if this rule has been disabled via RuleGuidanceCQD
  // the CQD is COMP_INT_77 and it represents a bitmap
  // below we check if the bit # 4 is ON
  if(CURRSTMT_OPTDEFAULTS->isRuleDisabled(4))
    return FALSE;

  if (NOT Rule::topMatch(expr,NULL))
    return FALSE;

  if (expr->getOperatorType() == REL_SHORTCUT_GROUPBY)
    return FALSE;

  // make sure we are looking at a group by node
  CMPASSERT(expr->getOperatorType() == REL_GROUPBY);

  GroupByAgg *bef = (GroupByAgg *) expr;

  // ---------------------------------------------------------------------
  // Fire this rule only for a leaf partial groupby (which has been created
  // thru the SplitGroupByRule).
  // ---------------------------------------------------------------------
  if (NOT bef->isAPartialGroupByLeaf())
    return FALSE;

  // Do not fire this rule for aggregates only
  if (bef->groupExpr().isEmpty())
    return FALSE;

  // If we get here, return TRUE
  return TRUE;

}

// -----------------------------------------------------------------------
// PartialGroupByOnTSJRule
//
// This rule performs the following transformation:
//
//           final GB            final GB
//              |                   |
//          partial GB             TSJ
//              |                 /    \
//             TSJ             Cut#0  partial GB
//           /    \                     |
//        Cut#0   Cut#1               Cut#1
//
// This transformation can be performed if the TSJ does not have the
// potential for increasing/reducing the data returned from the right
// child.  Thus, LEFT nested joins as well as semi-nested joins are
// disqualified from this transformation.  There is no restriction
// on the columns referenced in the partial GB, since the TSJ operator
// has the potential for "flowing" input values from the left child to
// the right child.  Thus, partial GB operator can group on values
// produced from either Cut#0 or Cut#1 above.
//
// -----------------------------------------------------------------------
#pragma nowarn(770)   // warning elimination
RelExpr * PartialGroupByOnTSJRule::nextSubstitute(RelExpr * before,
                                                  Context * /*context*/,
                                                  RuleSubstituteMemory * & /*memory*/)
{
  CMPASSERT(before->getOperatorType() == REL_GROUPBY);

  GroupByAgg *oldGB = (GroupByAgg *) before;
  Join *oldJoin = (Join *) before->child(0).getPtr();

  // Perform this transformation only for inner (non-semi) nested joins.
  if (NOT oldJoin->isInnerNonSemiJoin()) return NULL;

  GroupAttributes* TSJChild1GA =
    oldJoin->child(1).getGroupAttr();

  // Do not push the group by to the right leg of the TSJ if the
  // grouping columns are unique on the right leg of the TSJ.
  if (TSJChild1GA->isUnique(oldGB->groupExpr()))
    return NULL;

  // Do not push the group by to the right leg of the TSJ if the
  // right leg is not a scan.
  if (TSJChild1GA->getAvailableBtreeIndexes().isEmpty())
    return NULL;

  // HEURISTIC:
  // Do not push the group by down if the TSJ output number of rows
  // is less than or equal to 100 times the TSJ left child output number
  // of rows. This is because we only want to push the group by down
  // if each probe produces at least 100 rows, so that each partial
  // grouping operation will be on at least 100 rows. This is not
  // perfect - it doesn't take into account failed probes (i.e. probes
  // that do not find a match). But it should be good enough.
  if (oldJoin->getGroupAttr()->getResultCardinalityForEmptyInput() <=
      (oldJoin->child(0).getGroupAttr()->getResultCardinalityForEmptyInput()
       * 100.0))
    return NULL;

  // If the tsj performs any reduction of data, then do not perform
  // this transformation.  Pushing the partial groupby below the tsj
  // may result in incorrect aggregate results.  Check for any executor
  // preds that reside on the TSJ node that have not been pushed down.
  // Please note that VEG predicates (even though pushed) are still
  // retained on parent nodes.
  NABoolean executorPreds = FALSE;
  const ValueIdSet & selPreds = oldJoin->getSelectionPred();
  for (ValueId x = selPreds.init(); selPreds.next(x); selPreds.advance(x))
  {
    ItemExpr *ie = x.getItemExpr();
    if (ie->getOperatorType() != ITM_VEG_PREDICATE)
    {
      executorPreds = TRUE;
      return NULL;
    }
  }

  // If the groupby is not likely to fit in DP2 memory and does not result in
  //   significant data reduction, there is no reason to push it past the join.

  CostScalar afterGBCard =
    oldGB->getGroupAttr()->getResultCardinalityForEmptyInput();
  CostScalar beforeGBCard =
    oldJoin->getGroupAttr()->getResultCardinalityForEmptyInput();
  CostScalar reqGBReduction = CURRSTMT_OPTDEFAULTS->getReductionToPushGBPastTSJ();
  if (reqGBReduction > 0.0  AND  afterGBCard > beforeGBCard * reqGBReduction)
    return NULL;

  Join *newJoin = (Join *)oldJoin->copyTopNode(0, CmpCommon::statementHeap());
  GroupByAgg *newGB = (GroupByAgg *)oldGB->copyTopNode(0, CmpCommon::statementHeap());

  newJoin->setGroupAttr(before->getGroupAttr());
  newGB->setGroupAttr(new (CmpCommon::statementHeap()) GroupAttributes());

  // Remember that this partial GB has been pushed below a TSJ by this rule.
  newGB->gbAggPushedBelowTSJ() = TRUE;

  // produce join(cut(1), grby(cut(1)))
  newJoin->child(0) = oldJoin->child(0).getPtr();
  newJoin->child(1) = newGB;
  newGB->child(0)   = oldJoin->child(1).getPtr();

  // ---------------------------------------------------------------------
  // set up the group attributes and logical properties of the substitute
  // ---------------------------------------------------------------------
  newJoin->allocateAndPrimeGroupAttributes();

  // The pushed down partial groupby may require as potential inputs those
  // outputs produced by the left child of the TSJ.
  newGB->getGroupAttr()->addCharacteristicInputs
    (((RelExpr *)newJoin->child(0))->getGroupAttr()->getCharacteristicOutputs());

  // Refine the Characteristic Inputs and Outputs of the join
  newJoin->pushdownCoveredExpr
    (newJoin->getGroupAttr()->getCharacteristicOutputs(),
     newJoin->getGroupAttr()->getCharacteristicInputs(),
     newJoin->selectionPred());

  // pushdownCoveredExpr() is not called onthe GroupBy because
  // its child should be a CutOp; it is not legal to pushdown a
  // predicate and modify the Group Attributes of the Cascades GROUP
  // that it belongs to.

  // synthesize logical properties for the new GB node
  newGB->synthLogProp();
  newGB->getGroupAttr()->addToAvailableBtreeIndexes(
    newGB->child(0).getGroupAttr()->getAvailableBtreeIndexes());

  return newJoin;
} // PartialGroupByOnTSJRule::nextSubstitute()
#pragma warn(770)  // warning elimination


// -----------------------------------------------------------------------
// methods for class ShortCutGroupByRule
// -----------------------------------------------------------------------

ShortCutGroupByRule::~ShortCutGroupByRule() {}

NABoolean ShortCutGroupByRule::topMatch (RelExpr *expr,
                                         Context * /*context*/)
{
  if (NOT Rule::topMatch(expr,NULL))
    return FALSE;

  if (expr->getOperatorType() == REL_SHORTCUT_GROUPBY)
    return FALSE;

  // make sure we are looking at a group by node
  CMPASSERT(expr->getOperatorType() == REL_GROUPBY);

  // QSTUFF
  CMPASSERT(NOT(expr->getGroupAttr()->isStream() OR
                expr->getGroupAttr()->isEmbeddedUpdateOrDelete()));
  // QSTUFF

  // we are only interested in ShortCut aggregate here
  GroupByAgg *grbyagg = (GroupByAgg *) expr;
  const ValueIdSet &aggrs = grbyagg->aggregateExpr();

  if (! grbyagg->groupExpr().isEmpty()) return FALSE;

  if (aggrs.isEmpty()) return FALSE;

  ValueId aggr_valueid = aggrs.init();
  aggrs.next(aggr_valueid);

  ItemExpr *item_expr = aggr_valueid.getItemExpr();
  OperatorTypeEnum op = item_expr->getOperatorType();

  if ( op!=ITM_MIN AND op!= ITM_MAX AND op!= ITM_ANY_TRUE) return FALSE;
  //do not do min-max optimization if it is not a full group by or if
  //default switches it off
  if ( (op!=ITM_ANY_TRUE) AND (NOT grbyagg->isNotAPartialGroupBy()
	  OR NOT CURRSTMT_OPTDEFAULTS->isMinMaxConsidered()))
	  return FALSE;
  if (aggrs.entries()>1) return FALSE;

  // we are only interested in anytrue with inequality operator
  if(op==ITM_ANY_TRUE)
  {
    ItemExpr* anytrue_expr = item_expr->child(0);
    OperatorTypeEnum op = anytrue_expr->getOperatorType();

    // ---------------------------------------------------------------------
    // Perform this transformation if and only if the operand of the
    // anyTrue is a predicate. When the GroupBySplitRule fires, it
    // rewrites the anyTrue for staged evaluation and evaluates the
    // anyTrue(p) first, followed by anyTrue(anyTrue(p)). The second
    // aggregate should not be evalued using the ShortCutGroupBy.
    // ---------------------------------------------------------------------
    if ((NOT anytrue_expr->isAPredicate()) OR
        op == ITM_EQUAL OR op == ITM_NOT_EQUAL)
      return FALSE;
  }
  else
  {
	ItemExpr* simplifiedIE = item_expr->child(0)->simplifyOrderExpr();
	OperatorTypeEnum op = simplifiedIE->getOperatorType();
	if(op != ITM_VEG_REFERENCE AND
		op != ITM_BASECOLUMN AND op!=ITM_INDEXCOLUMN) return FALSE;

  }

  return TRUE;

} // ShortCutGroupByRule::topMatch()

RelExpr * ShortCutGroupByRule::nextSubstitute(RelExpr * before,
                                             Context * /*context*/,
                                             RuleSubstituteMemory *& /*memory*/)
{
  ShortCutGroupBy *result;
  GroupByAgg *bef = (GroupByAgg *) before;

  // create a shortcut groupby node
  result = new(CmpCommon::statementHeap()) ShortCutGroupBy(bef->child(0));

  // Copy over the groupbyagg private fields, then
  // call the relexpr copytopnode to copy over all common fields.
  (void) bef->copyTopNode(result, CmpCommon::statementHeap());

  // init new private members
  // the validation of anytrue_expr is done in topMatch
  const ValueIdSet &aggrs = result->aggregateExpr();

  ValueId aggrid = aggrs.init();
  aggrs.next(aggrid);

  ItemExpr * aggr = aggrid.getItemExpr();
  OperatorTypeEnum main_op = aggr->getOperatorType();
  if(main_op == ITM_MIN OR main_op == ITM_MAX)
  {
    // now set the group attributes of the result's top node
    result->setGroupAttr(bef->getGroupAttr());
    ItemExpr * min_max_expr = aggr->child(0);
    result->set_lhs(NULL);
    result->set_rhs(min_max_expr);
    result->setIsNullable(min_max_expr->getValueId().getType().supportsSQLnullLogical());
    result->setFirstNRows(1);
    if(main_op == ITM_MIN )
    {
      result->setOptForMin(TRUE);
      result->setOptForMax(FALSE);
    }
    else
    {
      result->setOptForMin(FALSE);
      result->setOptForMax(TRUE);
      if(result->isNullable()){
        Filter * filter = new(CmpCommon::statementHeap()) Filter(result->child(0));
	filter->setReattemptPushDown();
        result->child(0) = filter;
        ItemExpr * notNullExpr = new(CmpCommon::statementHeap())
                           UnLogic (ITM_IS_NOT_NULL,min_max_expr);
        notNullExpr->synthTypeAndValueId();

        // set the selection predicate for the filter
        filter->selectionPred() += notNullExpr->getValueId();

        result->allocateAndPrimeGroupAttributes();

        ValueIdSet expr =
            result->getGroupAttr()->getCharacteristicOutputs();

        ValueIdSet input = result->getGroupAttr()->getCharacteristicInputs();

        result->pushdownCoveredExpr(expr, input,
                                     result->selectionPred());
        filter->synthLogProp();

      }
    }

  }
  else
  {

  ItemExpr *anytrue_expr = aggr->child(0);
  ItemExpr *lhs = anytrue_expr->child(0);
  ItemExpr *rhs = anytrue_expr->child(1);
  OperatorTypeEnum op = anytrue_expr->getOperatorType();

  result->set_lhs(lhs);
  result->set_rhs(rhs);

  result->setIsNullable(lhs->getValueId().getType().supportsSQLnullLogical() OR
                        rhs->getValueId().getType().supportsSQLnullLogical());

  if (op == ITM_GREATER OR op == ITM_GREATER_EQ)
    {
      // now set the group attributes of the result's top node
      result->setGroupAttr(bef->getGroupAttr());
      result->setOptForMin(TRUE);
      result->setOptForMax(FALSE);


      if (result->isNullable() AND result->canApplyMdam())
        {
          // add a filter node with the selection predicate:
          //  (anytrue_expr is TRUE) OR (anytrue_expr is UNKNOWN)
          //  => (anytrue_expr is NOT FALSE)
          // introduce a filter node

          // note that without MDAM-like access method,
          // the introduction of the input node in the selection pred.
          // of the filter could result in materialization for each input

          Filter *filter = new(CmpCommon::statementHeap())
            Filter(result->child(0));
	  filter->setReattemptPushDown();
          result->child(0) = filter;

          // synthesis the selection predicate from anytrue_expr
          ItemExpr *rootptr = new(CmpCommon::statementHeap())
            UnLogic (ITM_IS_FALSE,
                     anytrue_expr);
          rootptr = new(CmpCommon::statementHeap()) UnLogic(ITM_NOT, rootptr);
          rootptr->synthTypeAndValueId();

          // set the selection predicate for the filter
          filter->selectionPred() += rootptr->getValueId();

          result->allocateAndPrimeGroupAttributes();

          ValueIdSet relExpr =
            result->getGroupAttr()->getCharacteristicOutputs();

          ValueIdSet input = result->getGroupAttr()->getCharacteristicInputs();

          result->pushdownCoveredExpr(relExpr, input,
                                      result->selectionPred() );

         }
    }
  else if (op == ITM_LESS OR op == ITM_LESS_EQ)
    {
      // do not set result group attributes to that of before, later we would
      // set MapValueIds group attributes to that of RelExpr before

      result->setOptForMin(FALSE);
      result->setOptForMax(TRUE);

      // get a copy of anytrue_aggr
      // and replace the operator by ITM_ANY_TRUE_MAX
      ItemExpr *new_anytrue_aggr =
        new(CmpCommon::statementHeap()) Aggregate(ITM_ANY_TRUE_MAX,
                      aggr->child(0));
      new_anytrue_aggr->synthTypeAndValueId();
      result->aggregateExpr().clear();
      result->aggregateExpr() += new_anytrue_aggr->getValueId();

      // mapping the new ValueId into the old one so that
      //   all the references of the old aggregate expression maps
      //   to the new ValueId
      // also MapAndRewrite the selection pred containing the
      //   old (upper) ValueId

      MapValueIds *mvi = new(CmpCommon::statementHeap()) MapValueIds(result);
      mvi->addMapEntry(aggr->getValueId(),       // upper ValueID
                       new_anytrue_aggr->getValueId());  // lower(new) ValueID

      result->selectionPred().clear();
      // map the selection predicates down over the map
      for (ValueId x = bef->selectionPred().init();
           bef->selectionPred().next(x);
           bef->selectionPred().advance(x))
        {
          result->selectionPred() +=
            x.getItemExpr()->mapAndRewrite(mvi->getMap(),TRUE);
        }

      // Genesis case: 10-010315-1747. Synthesizing MapValueId outputs correctly
      // MapValueIds should produce uppervalues as output, if required.
      // This also fixes genesis case 10-010320-1817.
      // ValueIdSet valuesForRewrite;

      mvi->setGroupAttr(bef->getGroupAttr());

      // set group attributes for children
      result->getGroupAttr()->addCharacteristicInputs(
             bef->getGroupAttr()->getCharacteristicInputs());

      ValueIdSet resultOutputs;

      // Map the outputs of the groupby to be in terms of the new values.
      //
      mvi->getMap().
        rewriteValueIdSetDown(bef->getGroupAttr()->getCharacteristicOutputs(),
                              resultOutputs);

      result->getGroupAttr()->addCharacteristicOutputs(resultOutputs);

      // Removed 10/10/16 as part of fix for TRAFODION-2127
      // These values were not used in MapValueIds::preCodeGen.
      // Could consider adding this if there are issue in preCodeGen.

      // bef->getValuesRequiredForEvaluatingAggregate(valuesForRewrite);
      // mvi->addValuesForVEGRewrite(valuesForRewrite);

      // perform synthesis on the new child node
      mvi->child(0)->synthLogProp();

      result = (ShortCutGroupBy *) mvi;

      // currently this rule does not use memory
      // CMPASSERT(memory == NULL);
    }
  else
    {
      result->setGroupAttr(bef->getGroupAttr());
    }
  }
  return result;
} // ShortCutGroupByRule::nextSubstitute()

NABoolean ShortCutGroupByRule::canMatchPattern(
            const RelExpr * /*pattern*/) const
{
  // The ShortCutGroupByRule can generate potentially several different
  // expressions.  So, just return TRUE for now.
  return TRUE;
}


// -----------------------------------------------------------------------
// methods for class CommonSubExprRule
// -----------------------------------------------------------------------

CommonSubExprRule::~CommonSubExprRule() {}

RelExpr * CommonSubExprRule::nextSubstitute(RelExpr * before,
                                            Context * /*context*/,
                                            RuleSubstituteMemory *& /*memory*/)
{
  // eliminate this node
  return before->child(0);
}

NABoolean CommonSubExprRule::canMatchPattern(
            const RelExpr * /*pattern*/) const
{
  // The CommonSubExprRule can potentially help with nearly any pattern
  return TRUE;
}


// -----------------------------------------------------------------------
// methods for class SampleScanRule
// -----------------------------------------------------------------------

// The promiseForOptimization for the SampleScanRule (CLUSTER
// sampling) has been raised to force the
// SampleScanRule::nextSubstitute (but not necessarily the topMatch
// method) to be applied before the PhysicalSampleRule.  This allows
// us to disable the PhysicalSampleRule if the SampleScanRule
// succeeds.  If the SampleScanRule fails, then the PhysicalSampleRule
// is applied.  In this case, if the sampling type is CLUSTER, the
// generator will detect that a plan for CLUSTER sampling could not be
// found.
//
Int32 SampleScanRule::promiseForOptimization(RelExpr * ,
                                             Guidance * ,
                                             Context * )
{
  return AlwaysBetterPromise;
}

NABoolean SampleScanRule::topMatch(RelExpr * expr,
                                   Context * context)
{
  // If the rule doesn't match the general pattern, quit.
  if (NOT Rule::topMatch(expr,context))
    return FALSE;

  const RelSample * befSample = (RelSample *)expr;

  // If Sample has a full balance expression, quit
  // If NOT random-relative sampling, quit.
  if (befSample->isSimpleRandomRelative() == FALSE)
    return FALSE;

  // For now disable the rule by default unless it is cluster
  // sampling. Allow firing through CQD
  if ((CmpCommon::getDefault(ALLOW_DP2_ROW_SAMPLING) != DF_ON) AND
      (befSample->getClusterSize() == -1))
    return FALSE;

  // If not in DP2, quit.
  // if (NOT context->getReqdPhysicalProperty()->executeInDP2())
  //   return FALSE;

  // else return true.
  return TRUE;
}


RelExpr * SampleScanRule::nextSubstitute(RelExpr * before,
                                         Context * context,
                                         RuleSubstituteMemory * &memory)
{
  CMPASSERT(before->getOperatorType() == REL_SAMPLE);

  RelSample * befSample = (RelSample *)before;
  Scan * befScan = (Scan *)(before->child(0).getPtr());

  CMPASSERT(befSample->isSimpleRandomRelative() == TRUE);

  // If scan is on VP table, then must be single partition scan
  if ((befScan->isVerticalPartitionTableScan() == TRUE) AND
      (befScan->isSingleVerticalPartitionScan() == FALSE))
    return NULL;

  // If scan has selection predicates, quit (for now). Ideally we
  // want to do sampling and selection. Revisit this later.
  if (befScan->selectionPred().entries() > 0)
    return NULL;

  // If the scan is in reverse order, quit.
  Scan * result = (Scan *)befScan->copyTopNode(0, CmpCommon::statementHeap());

  result->setGroupAttr(before->getGroupAttr());
  result->selectionPred() += befSample->selectionPred();

  result->samplePercent(befSample->getSamplePercent());
  result->sampledColumns() += befSample->sampledColumns();

  result->clusterSize(befSample->getClusterSize());

  // Do not copy index Info from befScan, but initialize it for the
  // sampleScan.  addIndexInfo() will only add the info for the
  // basetable accesspath since sampleScan does not use indexes.
  //
  result->addIndexInfo();

  // Recompute estimated logical properties
  // result->synthEstLogProp();

  // Indicate that we were able to successfully apply the
  // SampleScanRule for cluster sampling and thus should avoid
  // applying the PhysicalSampleRule.  The reason this is done is that
  // even if this rule succeeds the optimizer could still pick a plan
  // arising from the PhysicalSampleRule, but CLUSTER sampling can
  // only be implemented by a sample scan.
  //
  befSample->setSampleScanSucceeded(TRUE);

  return result;
}


//++MV,
NABoolean JoinToBushyTreeRule::topMatch (RelExpr * expr,
				       Context * context)
{
  // This rule fires only for joins that has insert to MV log as a right child (DrivingMvLogInsert)
  if (!expr->getInliningInfo().isDrivingMvLogInsert())
    return FALSE;

  if (NOT Rule::topMatch(expr,context))
    return FALSE;

  if (expr->getTolerateNonFatalError() == RelExpr::NOT_ATOMIC_)
     return FALSE;

  return TRUE;
}

// This rule implements a transformation that produces a bushy tree from left-linear tree of inner joins.
// This rule fires only for join that has insert to MV log as a right child (DrivingMvLogInsert)
//
// Pattern before the transformation, left linear tree
//
//                         Before - inner join
//                         /   bef_j_map      \
//                        / updateTableDesc_j  \
//            Inner Join #1                subtree 3
//          /  bef_j0_map      \
//         / updateTableDesc_j0 \
//     subtree 1           subtree 2
//
// Bushy tree produced by this rule
//
//             Result - inner join
//           /    bef_j0_map    \
//      subtree 1	      Inner Join #1
//                         /  bef_j_map       \
//                        / updateTableDesc_j  \
//                  subtree 2           subtree 3
//
RelExpr * JoinToBushyTreeRule::nextSubstitute(RelExpr * before,
					    Context * context,
					    RuleSubstituteMemory * & memory)
{
  // do the default pattern substitution logic
  Join *result = (Join *)Rule::nextSubstitute(before,NULL,memory);

  Join *join1 = (Join *)result->child(1).getPtr();

  Join *bef_join = (Join *)before;
  Join *bef_join0 = (Join *)bef_join->child(0).getPtr();

  // Copy updateSelectValudIdMaps and updateTableDesc from the
  // befor tree to after tree. Please see the location, the source
  // and the target of each copy in the transformation illustration
  // above. Note updateTableDesc_j0 is not copied because the right
  // child of the top join in the after tree is not an IUD node.
  //
  ValueIdMap* bef_j_map = bef_join->updateSelectValueIdMap();

  if ( bef_j_map) {
    join1 -> setUpdateSelectValueIdMap(
       new (CmpCommon::statementHeap()) ValueIdMap(*bef_j_map)
                                      );
  }
  join1 -> setUpdateTableDesc(bef_join->updateTableDesc());

  ValueIdMap* bef_j0_map = bef_join0->updateSelectValueIdMap();
  if ( bef_j0_map ) {
    result -> setUpdateSelectValueIdMap(
       new (CmpCommon::statementHeap()) ValueIdMap(*bef_j0_map)
                                       );
  }

  // Mark the join tree rooted at result to be NOT pushed down
  result->setAllowPushDown(FALSE);

  // Mark the join tree rooted at join1 to be pushed down
  join1->setAllowPushDown(TRUE);

  // don't try to apply the rule on the result
  result->contextInsensRules() += getNumber();

  // enable left shift rule and join commutativity rule on the result
  result->getInliningInfo().resetFlags(II_DrivingMvLogInsert);

  // pull up predicates
  result->selectionPred() += join1->selectionPred();
  join1->selectionPred().clear();

  // allocate a new Group Attributes for the join.
  join1->setGroupAttr(new (CmpCommon::statementHeap()) GroupAttributes);

  // add to it the input values of its children
  ValueIdSet joinInput = ((RelExpr *)join1->getChild(0))->getGroupAttr()->getCharacteristicInputs();
  joinInput += ((RelExpr *)join1->getChild(1))->getGroupAttr()->getCharacteristicInputs();
  join1->getGroupAttr()->setCharacteristicInputs(joinInput);

  // Compute the set of values that each child will potentially require
  // for evaluating expressions. Also compute the set of values that
  // the child is capable of producing as output.
  // This call will NOT effect the characteristic input and output
  // values of CutOps.
  result->allocateAndPrimeGroupAttributes();

  // Push down as many full or partial expressions that can be
  // computed by the children. Recompute the Group Attributes of
  // each child that is not a CutOp.
  result->pushdownCoveredExpr
            (result->getGroupAttr()->getCharacteristicOutputs(),
	     result->getGroupAttr()->getCharacteristicInputs(),
	     result->selectionPred());

  // synthesize the estimated logical properties for the new node and the result
  join1->synthLogProp();
  result->synthLogProp();

  return result;
}
//--MV

// -----------------------------------------------------------------------
// methods for class OnceGuidance
// -----------------------------------------------------------------------

OnceGuidance::~OnceGuidance() {}

OnceGuidance::OnceGuidance(NAUnsigned exceptRule, CollHeap* h) :
     allButOne_(h)
{
  // make a rule subset with all applicable rules, but leave the
  // one specified in the constructor argument out
  allButOne_ = *(GlobalRuleSet->applicableRules());
  allButOne_ -= exceptRule;
}

const RuleSubset * OnceGuidance::applicableRules()
{
  return &allButOne_;
}

// -----------------------------------------------------------------------
// methods for class StopGuidance
// -----------------------------------------------------------------------

StopGuidance::StopGuidance(CollHeap* h) :
  emptySet_(h)
{
}

StopGuidance::~StopGuidance() {}

const RuleSubset * StopGuidance::applicableRules()
{
  return &emptySet_;
}

// -----------------------------------------------------------------------
// methods for class FilterGuidance
// -----------------------------------------------------------------------
FilterGuidance::~FilterGuidance() {}

// -----------------------------------------------------------------------
// FilterGuidance:
//
//   Added to fix CR # 10-001006-2815.
//
//   Only allow the filter rules to be applied - i.e.
// FilterRule0, FilterRule1, and FilterRule2. Used when exploring
// the child of a filter so that double filter nodes - i.e. a
// filter that is a child of a filter - are processed correctly.
//
// This is necessary because FilterRule1, which is the rule that
// would process a filter over a filter (since a filter is a unary
// non-leaf operator), will refuse to do anything if it's child is
// a filter. It assumes that the filter child will be merged with
// it's child when exploring.  But the filter rule used to issue
// "StopGuidance", above, so this would never happen during exploring.
// It also never happens during optimization, because there is no
// implementation rule for a filter, and rules are only issued
// for the immediate child of an operator during optimization if the
// parent was actually implemented. Prior to adding this guidance
// class, once one filter ended up on top of another filter,
// the "double filter" could never be eliminated. One solution
// would be to issue no guidance at all, so all rules could fire.
// This was not done because it was assumed that the reason
// "StopGuidance" was issued was because applying all the rules to
// the child of a filter was causing a performance problem,
// since all the rules would have to be applied on the child
// again after the filter was merged with the child. By
// issuing a guidance that only allows the filter rules to fire,
// additional work will only be performed if we are indeed processing
// a "double filter". These were never being processed before, so
// it is not possible that this will result in any new unnecessary work.
//
//  NOTE added 11/14/00 : This solution did not work. Apparently, the
// reason the filter rule issued "Stop Guidance" was not strictly for
// performance reasons, but to cover up some horrible problem with
// cascades. After implementing this fix, certain queries with
// aggregates, group by, or having clauses caused what looked like
// an infinite loop in cascades during exploring. I can't figure out
// why, and so this fix is backed out. Instead, FilterRule1 is
// modified to eliminate one of the filters if it encounters a filter
// over a filter. FilterRule::guidanceForExploringChild() now
// issues StopGuidance once again instead of FilterGuidance.
// -----------------------------------------------------------------------
FilterGuidance::FilterGuidance(CollHeap* h) :
     filterRules_(h)
{
  //Solution 10-030930-0076
  //A filter cannot be pushed down onto a MultiJoin
  //Therefore make sure MJ enumeration rules fire
  //before filter push down onto a MJ.
  //The MultiJoin enumeration rules will create joins
  //onto which we can push down a filter.
  filterRules_ += MJEnumRuleNumber;
  filterRules_ += MJExpandRuleNumber;
  filterRules_ += MJStarJoinIRuleNumber;
  filterRules_ += MJStarJoinIIRuleNumber;
  filterRules_.intersectSet(*(GlobalRuleSet->applicableRules()));

  // make a rule subset with only the filter rules
  // (FilterRule0, FilterRule1, FilterRule2)
  filterRules_ += FilterRule0Number;
  filterRules_ += FilterRule1Number;
  filterRules_ += FilterRule2Number;
}

const RuleSubset * FilterGuidance::applicableRules()
{
  return &filterRules_;
}

// -----------------------------------------------------------------------
// Methods for class TSJUDRRule
// TSJUDRRule implements UDR join transformations
// -----------------------------------------------------------------------
NABoolean TSJUDRRule::topMatch (RelExpr * expr,
				Context * context)
{
  if (NOT Rule::topMatch(expr,context))
    return FALSE;

  if ( REL_CALLSP == expr->getOperatorType ())
  {
    // If the CallSP doesn't have a child node, which it gets
    // if it has a subquery or a UDF as one of its input parameters,
    // then we don't need to do this transformation.
    if ( (FALSE == ((CallSP *)expr)->hasSubquery ()) && 
         (FALSE == ((CallSP *)expr)->hasUDF ()) )
    {
	return FALSE;
    }

    if ( expr->isPhysical ())
      return FALSE;

    return TRUE;
  }

  return FALSE;
}

// Raj P - 10/2000
// Used by CALL statement for stored procedures for Java
// Called to transform the logical node to a physical node
RelExpr * TSJUDRRule::nextSubstitute(RelExpr * before,
				     Context *context,
				     RuleSubstituteMemory * & memory)
{
  Join *result = NULL;

  switch (before->getOperatorType())
  {
  case REL_CALLSP:
  {
    CallSP * oldCallSP = (CallSP *) before;
    
    RelExpr *newCallSP = oldCallSP->copyTopNode(0,CmpCommon::statementHeap());
    newCallSP->setOperatorType (REL_CALLSP);
    
    ((CallSP *) newCallSP)->setPhysical ();
    
    // Create the new TSJFlow expression
    result =  new (CmpCommon::statementHeap())
      Join(before->child(0),
           newCallSP,
           REL_TSJ,
           NULL,
           FALSE,
           FALSE,
           CmpCommon::statementHeap(),
           NULL,
           NULL);
    
    // Now set the group attributes of the result's top node.
    result->setGroupAttr(before->getGroupAttr());
    
    // Recompute the input values that each child requires as well as
    // the output values that each child is capable of producing
    result->allocateAndPrimeGroupAttributes();
    
    // Output values produced by the left child of the tsjUDRFlow
    // becomes the inputs for the right child of the tsjUDRFlow.
    RelExpr * leftChild = result->child(0);
    RelExpr * rightChild = result->child(1);
    rightChild->getGroupAttr()->addCharacteristicInputs
      (((RelExpr *)before->child(0))->getGroupAttr()->getCharacteristicOutputs());
    
    // Push down as many full or partial expressions as can be
    // computed by the children.
    result->pushdownCoveredExpr(result->getGroupAttr()->getCharacteristicOutputs(),
                                result->getGroupAttr()->getCharacteristicInputs(),
                                result->selectionPred());
    
    // Synthesize logical properties for this new leaf generic update node.
    ((CallSP *)rightChild)->synthLogProp();
    
    result->setBlockStmt( before->isinBlockStmt() );
    break;
  }

  default:
    CMPASSERT(0);
    break;
  } // switch

  return result;

} // TSJUDRRule::nextSubstitute()

NABoolean TSJUDRRule::isContextSensitive () const
{
  return FALSE;
}

// methods for class HbaseAccessRule
// -----------------------------------------------------------------------

//HbaseScanRule::~HbaseScanRule() {} // LCOV_EXCL_LINE
//
//NABoolean HbaseScanRule::topMatch(RelExpr * relExpr, Context *context)
//{
//  if (NOT Rule::topMatch(relExpr,context))
//    return FALSE;
//
//  Scan * scan= (Scan *) relExpr;
//  if (scan->getTableDesc()->getNATable()->isHbaseTable() == FALSE)
//    return FALSE;
//
//  // Check for required physical properties that require an enforcer
//  // operator to succeed.
//  if (relExpr->rppRequiresEnforcer(context->getReqdPhysicalProperty()))
//    return FALSE;
//
//  return TRUE;
//
//}
//
//
//RelExpr * HbaseScanRule::nextSubstitute(RelExpr * before,
//                                              Context * /*context*/,
//                                              RuleSubstituteMemory *& /*mem*/)
//{
//  CMPASSERT(before->getOperatorType() == REL_SCAN);
//  Scan* bef = (Scan*) before;
//
//  // Simply copy the contents of the HbaseAccess from the before pattern.
//  HbaseAccess *result = new(CmpCommon::statementHeap()) 
//                 HbaseAccess(bef->getTableName(), bef->getTableDesc());
//
//  bef->copyTopNode(result, CmpCommon::statementHeap());
//
//  // now set the group attributes of the result's top node
//  result->selectionPred() = bef->selectionPred();
//  result->setGroupAttr(before->getGroupAttr());
//
//  return result;
//}
//
