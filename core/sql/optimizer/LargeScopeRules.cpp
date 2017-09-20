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
* File:         LargeScopeRules.cpp
* Description:  Large Scope Rules classes and methods
*
*
* Created:      6/10/2003
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "GroupAttr.h"
#include "TransRule.h"
#include "Analyzer.h"
#include "MultiJoin.h"
#include "AppliedStatMan.h"
#include "opt.h"

extern THREAD_P NAUnsigned              MJEnumRuleNumber;
extern THREAD_P NAUnsigned              MJStarJoinIRuleNumber;
extern THREAD_P NAUnsigned              MJStarJoinIIRuleNumber;
extern THREAD_P NAUnsigned              RoutineJoinToTSJRuleNumber;

// Excluding MJExpandRule related code since it is there for testing purposes
// MJExpandRule is not exercised during normal execution
// -----------------------------------------------------------------------
// methods for MJExpandRule
// -----------------------------------------------------------------------

NABoolean MJExpandRule::topMatch (RelExpr * expr,
                                           Context * context)
{
  if (NOT expr->getOperator().match(REL_MULTI_JOIN))
    return FALSE;

  // Get a handle to the MultiJoin on which we are performing the topMatch()
  MultiJoin* mjoin = (MultiJoin*) expr;

  // should we enumrate the user specified join order in pass 2 among other
  // join orders
  // Return FALSE if any one of the following is FALSE:
  // * check if CQD is ON
  // * check if optimization pass 2
  // * check if this is the top MultiJoin, i.e. MultiJoin that represents the
  //   whole JBB
  if (CmpCommon::getDefault(MULTI_JOIN_CONSIDER_INITIAL_JOIN_ORDER) == DF_OFF)
    return FALSE;

  if(GlobalRuleSet->getCurrentPassNumber() != 2)
    return FALSE;

  if(mjoin->getJBBSubset().getJBBCs() !=
     mjoin->getJBBSubset().getJBB()->getJBBCs())
    return FALSE;

  return TRUE;
}

RelExpr * MJExpandRule::nextSubstitute(RelExpr * before,
                                                Context *,
                                                RuleSubstituteMemory * &memory)
{
  MultiJoin * mjoin = (MultiJoin *) before;

  // Make sure GroupAnalysis localJBBView is the same as mjoin's JBBSubset.
  CMPASSERT(mjoin->getJBBSubset() ==
            *(mjoin->getGroupAttr()->getGroupAnalysis()->getLocalJBBView()));

  NABoolean fixJoinOrder = TRUE;
  NABoolean createPriviledgedJoins = TRUE;

  Join* result = mjoin->leftLinearize(fixJoinOrder, createPriviledgedJoins);
  // synth the join
  result->synthLogProp();
  // synth the left child too
  result->child(0)->synthLogProp();

  return result;
}

Int32 MJExpandRule::promiseForOptimization (RelExpr * expr,
            Guidance *,
            Context *)
{
  // should be higher than the promise for MJEnumRule
  return DefaultTransRulePromise + 1;
}

// -----------------------------------------------------------------------
// methods for MVQRRule
// -----------------------------------------------------------------------

NABoolean MVQRRule::topMatch (RelExpr * expr,
                              Context * context)
{
  // For optimization levels below the medium level we do not run the MVQRRule
  if (CURRSTMT_OPTDEFAULTS->optLevel() < OptDefaults::MEDIUM) 
    return FALSE;

  // rule is disabled
  if (CmpCommon::getDefaultLong(MVQR_REWRITE_LEVEL) < 1) 
    return FALSE;

  if (NOT expr->getOperator().match(REL_MULTI_JOIN))
    return FALSE;

  // conditions:
  // 1- do we have any matching mvs
  // 2- have we already processed them
  if ( (((MultiJoin *) expr)->getJBBSubset().getJBBSubsetAnalysis()->getMatchingMVs().entries()) && 
       !(((MultiJoin *) expr)->getJBBSubset().getJBBSubsetAnalysis()->getMatchingMVs()[0]->alreadyOptimized())
     ) 
    return TRUE;

  return FALSE;
}

RelExpr * MVQRRule::nextSubstitute(RelExpr * before,
                                   Context *,
                                   RuleSubstituteMemory * &memory)
{

  RelExpr *result;

  MultiJoin * mjoin = (MultiJoin *) before;

  if (memory == NULL)
  {
      // -----------------------------------------------------------------
      // this is the first call, create info on all matches
      // -----------------------------------------------------------------

     // Make sure GroupAnalysis localJBBView is the same as mjoin's JBBSubset.
     CMPASSERT(mjoin->getJBBSubset() ==
               *(mjoin->getGroupAttr()->getGroupAnalysis()->getLocalJBBView()));

     CollIndex numMatches = mjoin->getJBBSubset().getJBBSubsetAnalysis()->getMatchingMVs().entries();
    
     if (numMatches > 0)
     {
         // allocate a new memory for multiple substitutes
         memory = new (CmpCommon::statementHeap())
           RuleSubstituteMemory(CmpCommon::statementHeap());

        for (CollIndex matchIndex = 0; matchIndex < numMatches; matchIndex++)
        {
             // get the next match
             MVMatchPtr match = mjoin->getJBBSubset().getJBBSubsetAnalysis()->getMatchingMVs()[matchIndex];
             match->setAlreadyOptimized();
    
             RelExpr *matchExpr = match->getMvRelExprTree();
   
             // now set the group attributes of the result's top node
             matchExpr->setGroupAttr(before->getGroupAttr());
   
             // insert the match into the substitute memory
             memory->insert(matchExpr);
        } // for each match
     } // if (numMatches > 0)
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
// methods for MVQRScanRule
// -----------------------------------------------------------------------

NABoolean MVQRScanRule::topMatch (RelExpr * expr,
                                  Context * context)
{
  // For optimization levels below the medium level we do not run the MVQRScanRule
  if (CURRSTMT_OPTDEFAULTS->optLevel() < OptDefaults::MEDIUM)
    return FALSE;

  // rule is disabled
  if (CmpCommon::getDefaultLong(MVQR_REWRITE_LEVEL) < 1)
    return FALSE;

  // the case of an aggregate query on a single table, the matching information
  // is included part the JBBSubset
  if ((expr->getOperator().match(REL_SCAN) &&
      ((Scan *) expr)->getJBBSubsetAnalysis() &&  
      ((Scan *) expr)->getJBBSubsetAnalysis()->getMatchingMVs().entries()))
  {
    // DebugBreak();
    return TRUE;
  }

  // for the case of a simple query on a single table, the matching information 
  // can be anchored at the SCAN operator
  if ((expr->getOperator().match(REL_SCAN) &&
      ((Scan *) expr)->getMatchingMVs().entries()))
  {
    // DebugBreak();
    return TRUE;
  }

  

  return FALSE;
}

RelExpr * MVQRScanRule::nextSubstitute(RelExpr * before,
                                   Context *,
                                   RuleSubstituteMemory * &memory)
{

  RelExpr *result = NULL;

  Scan * scan = (Scan *) before;

  if (memory == NULL)
  {
     // -----------------------------------------------------------------
     // this is the first call, create info on all matches
     // -----------------------------------------------------------------

     JBBSubsetAnalysis *jbbSubsetAnalysis = scan->getJBBSubsetAnalysis();

     NAList<MVMatch*> matchingMVs(STMTHEAP);

     // the case of an aggregate query on a single table
     if (jbbSubsetAnalysis)
     {
        matchingMVs =  jbbSubsetAnalysis->getMatchingMVs();
     }
     else // else it is a simple non-aggregate query on a single table
     {
        matchingMVs = scan->getMatchingMVs();
     }

     CollIndex numMatches = matchingMVs.entries();
     if (numMatches > 0)
     {
         // allocate a new memory for multiple substitutes
         memory = new (CmpCommon::statementHeap())
           RuleSubstituteMemory(CmpCommon::statementHeap());

        for (CollIndex matchIndex = 0; matchIndex < numMatches; matchIndex++)
        {
             // get the next match
             MVMatchPtr match = matchingMVs[matchIndex];
    
             RelExpr *matchExpr = match->getMvRelExprTree();
   
             // now set the group attributes of the result's top node
             matchExpr->setGroupAttr(before->getGroupAttr());
   
             // insert the match into the substitute memory
             memory->insert(matchExpr);
        } // for each match
     } // if (numMatches > 0)
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
// methods for MJEnumRule
// -----------------------------------------------------------------------

NABoolean MJEnumRule::topMatch (RelExpr * expr,
                                Context * context)
{
  // For optimization levels below the medium level we do not run the MJEnumRule
  if (CURRSTMT_OPTDEFAULTS->optLevel() < OptDefaults::MEDIUM)
    return FALSE;
  if (NOT expr->getOperator().match(REL_MULTI_JOIN))
    return FALSE;

  // if any selective LSR plan is forced (currently only star join) then we should
  // not fire the MJEnumRule
  if(CmpCommon::getDefault(DIMENSIONAL_QUERY_OPTIMIZATION) == DF_ON)
    return FALSE;

  // If this is a very complex query, we would rather not do MJEnumRule and
  // give MJPrimeTableRule plans better chance to finish. We will take a complexity
  // of a n-way join here where n is set by CURRSTMT_OPTDEFAULTS->getMJEnumLimit()
  // We only do this for optimization level Medium.
  Int32 base = CURRSTMT_OPTDEFAULTS->getMJEnumLimit() ;
  if ((CURRSTMT_OPTDEFAULTS->optLevel() == OptDefaults::MEDIUM) AND
      (CURRSTMT_OPTDEFAULTS->getQueryComplexity() > base * pow(2,base-1)) AND
      (CmpCommon::getDefault(COMP_BOOL_2) == DF_OFF)) // make sure PTrule is ON
    return FALSE;

  MultiJoin* mjoin = (MultiJoin*) expr;

  // The Enum rule fires only on the top MultiJoin in a JBB
  // and following recursive calls which are marked by the scheduledLSRs set.
  if (mjoin->getJBBSubset().getJBBCs() != mjoin->getJBBSubset().getJBB()->getJBBCs() AND
      !mjoin->scheduledLSRs().contains(MJEnumRuleNumber))
    return FALSE;

  //get the StarBDRuleConfidence
  Int32 starBDConfidence = mjoin->getLSRConfidence()->getStarBDRuleConfidence();

  if((starBDConfidence != -1)&&
     (starBDConfidence > ((ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_15)))&&
     (mjoin->getJBBSubset().getJBBCs().entries() > 4))
    return FALSE;

  return TRUE;
}

RelExpr * MJEnumRule::nextSubstitute(RelExpr * before,
                                     Context *,
                                     RuleSubstituteMemory * &memory)
{
  RelExpr *result = NULL;
  MultiJoin * mjoin = (MultiJoin *) before;

  // Make sure GroupAnalysis localJBBView is the same as mjoin's JBBSubset.
  CMPASSERT(mjoin->getJBBSubset() ==
            *(mjoin->getGroupAttr()->getGroupAnalysis()->getLocalJBBView()));

  if (memory == NULL)
  {

    // allocate a new memory for multiple substitutes
    memory = new (CmpCommon::statementHeap())
      RuleSubstituteMemory(CmpCommon::statementHeap());

    const CANodeIdSet & jbbcs = mjoin->getJBBSubset().getJBBCs();
    CMPASSERT (mjoin->getJBBSubset().getGB() == NULL_CA_ID)

    Lng32 mySubgraphs = mjoin->getJBBSubset().numConnectedSubgraphs();

    Int32 numSubstitutes = 0; // number of substitutes enumerated

    // cardinality of the joins produced by this rule
    CostScalar joinCardinality =
      before->getGroupAttr()->getResultCardinalityForEmptyInput();

    NABoolean doSortBasedOnDataFlow =
      (CmpCommon::getDefault(COMP_BOOL_116) == DF_OFF);

    // Data flow optimization
    CostScalar childrenFlow = mjoin->getChildrenDataFlow();
#pragma nowarn(1506)   // warning elimination
    const Lng32 numChildren = jbbcs.entries();
#pragma warn(1506)  // warning elimination
    Lng32 childIter = -1; // temp iterator. *NOT* the same as child index
    RelExpr** potentialSubstitutes = new (CmpCommon::statementHeap()) RelExpr*[numChildren];
    CostScalar* substituteMetric = new (CmpCommon::statementHeap()) CostScalar[numChildren];
    CANodeId * substituteRightChild = new (CmpCommon::statementHeap()) CANodeId[numChildren];

    // inspectors ignore this part
    CostScalar minSubstituteMetric = -1;
    CostScalar* substituteMetric2 = new (CmpCommon::statementHeap()) CostScalar[numChildren];
    CostScalar minSubstituteMetric2 = -1;
    CostScalar nextMinSubstituteMetric2 = -1;
    // inspectors ignore ends

    // is there a multiplier present
    NABoolean multiplierPresent = FALSE;

    // is there are jbbc that provides an equalizing join
    NABoolean guaranteedEqualizerPresent = FALSE;

#ifdef _DEBUG
    if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
         CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
    {
      CURRCONTEXT_OPTDEBUG->stream() << "MJEnum Rule Application on " << mjoin->getJBBSubset().getText() << endl;
      CURRCONTEXT_OPTDEBUG->stream() << "Level " << mjoin->getArity() << ": " << childrenFlow.value() << endl;
    }
#endif

    // allow cross product control if:
    // 1) cross_product_control is active AND
    // 2) multijoin has >1 base table OR multijoin arity > 5
    NABoolean allowCrossProductControl =
      (CURRSTMT_OPTDEFAULTS->isCrossProductControlEnabled()) AND
      (mjoin->getGroupAttr()->getGroupAnalysis()->
       getAllSubtreeTables().entries() > 1 OR numChildren > 5);

    // enumerate the joins - Begin

    CANodeId i;
    for (i = jbbcs.init(); jbbcs.next(i); jbbcs.advance(i))
    {
      // pick this child to make right child of join
      CANodeId jbbcRight = i;
      JBBC * rightJBBC = i.getNodeAnalysis()->getJBBC();
      Join * rightJBBCParent = rightJBBC->getOriginalParentJoin();

      CANodeIdSet right(jbbcRight);
      CANodeIdSet left(jbbcs);
      left -= jbbcRight;

      JBBSubset * leftSubset = left.jbbcsToJBBSubset();

      if(leftSubset && !leftSubset->legal())
        continue;

      if(rightJBBCParent && rightJBBCParent->isRoutineJoin())
         // && (jbbcs.entries()>2)
      
      {
        CANodeIdSet jbbcsProvidingInput = rightJBBC->getJBBCsProvidingInput();
        CANodeIdSet rightNodes = right;
        rightNodes += jbbcsProvidingInput;
        rightNodes.intersectSet(jbbcs);
        
        if((rightNodes.entries() != jbbcs.entries()) &&
           jbbcsProvidingInput.entries())
        {
          jbbcsProvidingInput.intersectSet(jbbcs);
          JBBSubset * inputSubset = jbbcsProvidingInput.jbbcsToJBBSubset();
          CANodeIdSet otherSet = left;
          otherSet -= jbbcsProvidingInput;
          JBBSubset * otherJBBSubset = otherSet.jbbcsToJBBSubset();
        
          CANodeIdSet jbbcsJoinedToOther = otherJBBSubset->getJoinedJBBCs();
          CANodeIdSet jbbcsNeededByOther = otherJBBSubset->getPredecessorJBBCs();
          CANodeIdSet jbbcsNeededByInput = inputSubset->getPredecessorJBBCs();
        
          jbbcsJoinedToOther.intersectSet(jbbcsProvidingInput);
          jbbcsNeededByOther.intersectSet(jbbcsProvidingInput);
          jbbcsNeededByInput.intersectSet(otherSet);
        
          if((!jbbcsJoinedToOther.entries()) &&
             (!jbbcsNeededByOther.entries()) &&
             (!jbbcsNeededByInput.entries()))
          {
            if(otherJBBSubset->legal() && 
               rightNodes.jbbcsToJBBSubset()->legal())
            {
              right += jbbcsProvidingInput;
              left -= jbbcsProvidingInput;
              leftSubset = left.jbbcsToJBBSubset();
            }
          }
        }
        
        /*
        if(jbbcsProvidingInput.entries() == 1)
        {
          CANodeId nodeProvidingInput = jbbcsProvidingInput.getFirst();
          JBBC * jbbcProvidingInput =
            nodeProvidingInput.getNodeAnalysis()->getJBBC();
          CANodeIdSet jbbcsJoinedToInput = jbbcProvidingInput->getJoinedJBBCs();
          if(jbbcsJoinedToInput.entries() < 1)
          {
            CANodeIdSet jbbcsNeededByLeft = 
              leftSubset->getPredecessorJBBCs();
            if(!jbbcsNeededByLeft.contains(nodeProvidingInput))
              right += nodeProvidingInput;
              left -= nodeProvidingInput;
              leftSubset = left.jbbcsToJBBSubset();
          }
        }
        */
      }
      
      JBBSubset * rightSubset = right.jbbcsToJBBSubset();

      Lng32 newSubgraphs = leftSubset->numConnectedSubgraphs();

      // xxx only counting joinPreds here
      // do this only for inner-nonSemi-nonTSJ joins
      if(((!rightJBBCParent) ||
          rightJBBCParent->isInnerNonSemiNonTSJJoin()) &&
          (rightSubset->joinPredsWithOther(*(leftSubset)).entries() == 0))
        newSubgraphs++;

      if ((newSubgraphs > mySubgraphs) && allowCrossProductControl)
      {
#ifdef _DEBUG
        if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
             CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
        {
          CURRCONTEXT_OPTDEBUG->stream() << " Unnecessary Cross Product when joining right child" << (CollIndex) i << endl;
        }
#endif
        continue;
      }

      if (jbbcRight.getNodeAnalysis()->getJBBC()->isFullOuterJoinOrTSJJBBC() &&
          !(jbbcRight.getNodeAnalysis()->getJBBC()->getOriginalParentJoin()))
      {
        continue;
      }

      Join* substitute = mjoin->splitSubset(*(leftSubset),
                                            *(rightSubset));

      // this should not happen, but just in case
      if (!substitute)
        continue;

      potentialSubstitutes[numSubstitutes] = substitute;

      substitute->child(0)->synthLogProp();
      
      // synthesize logical properties if right child is a multi join
      if(rightSubset->getJBBCs().entries() > 1)
        substitute->child(1)->synthLogProp();

      // Schedule the enumeration rule on the right child if applicable
      if (rightSubset->getJBBCs().entries() > 1)
        ((MultiJoin*)(substitute->getChild(1)))->scheduledLSRs() += MJEnumRuleNumber;

      // Schedule the enumeration rule on the right child if applicable
      if (leftSubset->getJBBCs().entries() > 1)
        ((MultiJoin*)(substitute->getChild(0)))->scheduledLSRs() += MJEnumRuleNumber;

      UInt32 minRecordLength = (ActiveSchemaDB()->getDefaults())\
                                     .getAsLong(COMP_INT_50);

      CostScalar leftChildFlow = // Flow from left MultiJoin
        substitute->child(0)->getGroupAttr()->getResultCardinalityForEmptyInput() *
        MAXOF(substitute->child(0)->getGroupAttr()->getRecordLength(), minRecordLength);

      substituteMetric[numSubstitutes] = childrenFlow + leftChildFlow;

      substituteMetric2[numSubstitutes] = leftChildFlow;

      substituteRightChild[numSubstitutes] = i;

#ifdef _DEBUG
        if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
             CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
        {
          CURRCONTEXT_OPTDEBUG->stream() << "Metric1 for Right Child " << (CollIndex) i <<
          " is "<< substituteMetric[numSubstitutes].value() << endl;
          CURRCONTEXT_OPTDEBUG->stream() << "Metric2 for Right Child " << (CollIndex) i <<
          " is "<< substituteMetric2[numSubstitutes].value() << endl;
        }
#endif

      // Remembering the substitutes with min metric
      if ((substituteMetric[numSubstitutes] < minSubstituteMetric) ||
          (minSubstituteMetric == -1))
      {
        minSubstituteMetric = substituteMetric[numSubstitutes];
      }

      // inspectors ignore this part
      // Remembering the substitutes with lowest two metrics
      if ((substituteMetric2[numSubstitutes] < minSubstituteMetric2) ||
          (minSubstituteMetric2 == -1))
      {
        nextMinSubstituteMetric2 = minSubstituteMetric2;
        minSubstituteMetric2 = substituteMetric2[numSubstitutes];
      }
      else if ((substituteMetric2[numSubstitutes] < nextMinSubstituteMetric2) ||
               (nextMinSubstituteMetric2 == -1))
      {
        nextMinSubstituteMetric2 = substituteMetric2[numSubstitutes];
      }
      // inspectors ignore ends

      if(rightJBBC->isGuaranteedEqualizer())
        guaranteedEqualizerPresent = TRUE;

      // get substitute left child card
      CostScalar leftCard =
        potentialSubstitutes[numSubstitutes]->
          child(0)->getGroupAttr()->
            getResultCardinalityForEmptyInput();

      if((leftCard < joinCardinality) &&
         (rightJBBC->getJoinedJBBCs().entries()==1) &&
         (jbbcs.contains(rightJBBC->getJoinedJBBCs())))
        multiplierPresent = TRUE;

      numSubstitutes++;
    }
    // enumerate the joins - End

    // dataflow based pruning - Begin

    // Data Flow Optimization Fudge Factors
    CostScalar DATA_FLOW_FACTOR_1
      ((ActiveSchemaDB()->getDefaults()).getAsDouble(COMP_FLOAT_7));
    const CostScalar DATA_FLOW_FACTOR_2(1000);

    Int32 numPrunedSubstitutes = 0;

    // Variables used to enumerate only one substitute for multipliers.
    // Only one substitute with an multiplier is enumerated. The substitute
    // that produces the lowest data flow is chosen. Currently multipliers
    // connected to only a single tables are considered.
    // index of the equalizer with the lowest dataflow
    CollIndex multiplierWithLowestDFIdx = -1;
    // lowest dataflow of the substitute using an equalizer
    CostScalar multiplierLowestDF = -1;

    // Variables used to enumerate only one substitute for equalizers.
    // Only one substitute with an equalizer is enumerated. The substitute
    // that produces the lowest data flow is chosen. Currently children
    // connected via a left join on a unique column are considered equalizers,
    // since they will neither increase nor decrease the # or rows coming in
    // from the left side of the join.
    // index of the equalizer with the lowest dataflow
    CollIndex equalizerWithLowestDFIdx = -1;
    // lowest dataflow of the substitute using an equalizer
    CostScalar equalizerLowestDF = -1;

    for (childIter = 0;
         childIter < numSubstitutes;
         childIter++)
    {
      RelExpr* substitute = potentialSubstitutes[childIter];

      if (!substitute)
      {
        continue;
      }

      // get the right child JBBC
      CANodeId jbbcRight = substituteRightChild[childIter];
      JBBC * rightJBBC = jbbcRight.getNodeAnalysis()->getJBBC();

      // get left child card
      CostScalar lCard =
        potentialSubstitutes[childIter]->
          child(0)->getGroupAttr()->
            getResultCardinalityForEmptyInput();

      if((mjoin->getArity() > 2) &&
         multiplierPresent &&
         ((ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_79) > 1))
      {
        if((lCard < joinCardinality)&&
           (rightJBBC->getJoinedJBBCs().entries()==1) &&
           (jbbcs.contains(rightJBBC->getJoinedJBBCs())))
        {
          CollIndex multiplierIdx = childIter;
          CostScalar multiplierDF = substituteMetric[childIter];

          // first equalizer, don't prune
          if(multiplierWithLowestDFIdx == -1)
          {
            multiplierWithLowestDFIdx = multiplierIdx;
            multiplierLowestDF = multiplierDF;
          }
          else{
            // prune the equalizer with a worse dataflow
            if(multiplierDF < multiplierLowestDF)
            {
              potentialSubstitutes[multiplierWithLowestDFIdx] = NULL;
              multiplierWithLowestDFIdx = multiplierIdx;
              multiplierLowestDF = multiplierDF;
            }
            else{
              potentialSubstitutes[multiplierIdx] = NULL;
            }
            numPrunedSubstitutes++;
          }
        }
        else{
          potentialSubstitutes[childIter] = NULL;
          numPrunedSubstitutes++;
        }
        continue;
      }

      if((mjoin->getArity() > 2) &&
         guaranteedEqualizerPresent &&
         ((ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_79) > 1))
      {
        if(rightJBBC->isGuaranteedEqualizer())
        {
          CollIndex equalizerIdx = childIter;
          CostScalar equalizerDF = substituteMetric[childIter];

          // first equalizer, don't prune
          if(equalizerWithLowestDFIdx == -1)
          {
            equalizerWithLowestDFIdx = equalizerIdx;
            equalizerLowestDF = equalizerDF;
          }
          else{
            // prune the equalizer with a worse dataflow
            if(equalizerDF < equalizerLowestDF)
            {
              potentialSubstitutes[equalizerWithLowestDFIdx] = NULL;
              equalizerWithLowestDFIdx = equalizerIdx;
              equalizerLowestDF = equalizerDF;
            }
            else{
              potentialSubstitutes[equalizerIdx] = NULL;
            }
            numPrunedSubstitutes++;
          }
        }
        else{
          potentialSubstitutes[childIter] = NULL;
          numPrunedSubstitutes++;
        }
        continue;
      }

      if ((mjoin->getArity() > 2) &&
          (CURRSTMT_OPTDEFAULTS->optimizerHeuristic5()) && // Data Flow Optimization
          (substituteMetric[childIter] > DATA_FLOW_FACTOR_1 * minSubstituteMetric + DATA_FLOW_FACTOR_2))
      {
#ifdef _DEBUG
        if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
             CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
        {
          CURRCONTEXT_OPTDEBUG->stream() << "For  right child "
            << (CollIndex) substituteRightChild[childIter]
            << " " << substituteMetric[childIter].value()
            << " >> "<< minSubstituteMetric.value() << endl;
        }
#endif
        potentialSubstitutes[childIter] = NULL;
        numPrunedSubstitutes++;
        continue;
      }

      // The if condition below is unneccesary
      // Aggressive Data Flow Heuristic.
      if ((((ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_51) >= 2) &&
           (substituteMetric2[childIter] > nextMinSubstituteMetric2)) ||
          (((ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_51) == 1) &&
           (substituteMetric2[childIter] > minSubstituteMetric2)))
      {

        if (mjoin->getArity() > 2)
        {
#ifdef _DEBUG
          if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
               CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
          {
            CURRCONTEXT_OPTDEBUG->stream() << "For right child "
               << (CollIndex) substituteRightChild[childIter]
               << " " << substituteMetric2[childIter].value()
               << " ---- "<< nextMinSubstituteMetric2.value()
               << " ---- "<< minSubstituteMetric2.value() << endl;
          }
#endif
          potentialSubstitutes[childIter] = NULL;
          numPrunedSubstitutes++;
          continue;
        }
      }

      // prune substitutes that involve an equalizer
      if ((mjoin->getArity() > 2) &&
          rightJBBC->isGuaranteedEqualizer() &&
          ((ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_79) > 0))
      {
        CollIndex equalizerIdx = childIter;
        CostScalar equalizerDF = substituteMetric[childIter];

        // first equalizer, don't prune
        if(equalizerWithLowestDFIdx == -1)
        {
          equalizerWithLowestDFIdx = equalizerIdx;
          equalizerLowestDF = equalizerDF;
        }
        else{
          // prune the equalizer with a worse dataflow
          if(equalizerDF < equalizerLowestDF)
          {
            potentialSubstitutes[equalizerWithLowestDFIdx] = NULL;
            equalizerWithLowestDFIdx = equalizerIdx;
            equalizerLowestDF = equalizerDF;
          }
          else{
            potentialSubstitutes[equalizerIdx] = NULL;
          }
          numPrunedSubstitutes++;
          continue;
        }
      }
    }
    // dataflow based pruning - End

    // Sort based on dataflow - Begin

    // The pruning above could have left 'holes' in the potentialSubstitutes
    // array, the sorting below will 'compact' the array.

    // following four arrays are used to sort the substitutes by
    // substituteMetric
    RelExpr** sortedPotentialSubstitutes =
      new (CmpCommon::statementHeap()) RelExpr*[numSubstitutes];
    CostScalar* sortedSubstituteMetric =
      new (CmpCommon::statementHeap()) CostScalar[numSubstitutes];
    CostScalar* sortedSubstituteMetric2 =
      new (CmpCommon::statementHeap()) CostScalar[numSubstitutes];
    CANodeId* sortedSubstituteRightChild =
      new (CmpCommon::statementHeap()) CANodeId[numSubstitutes];

    for (childIter = 0; childIter < numSubstitutes; childIter++)
    {
      RelExpr * largestChild = NULL;
      Int32 largestChildLoc = -1;
      CostScalar largestChildMetric = -1;
      CANodeId largestChildId = NULL_CA_ID;

      for (Int32 childIter2 = 0; childIter2 < numSubstitutes; childIter2++)
      {
        RelExpr* substitute = potentialSubstitutes[childIter2];
        CANodeId childId = substituteRightChild[childIter2];
        if (!substitute)
        {
          continue;
        }

        CostScalar currentChildMetric = substituteMetric[childIter2];

        if (currentChildMetric > largestChildMetric)
        {
          largestChild = substitute;
          largestChildLoc = childIter2;
          largestChildMetric = substituteMetric[childIter2];
          largestChildId = childId;
        }
      }

      if(largestChild)
      {
        potentialSubstitutes[largestChildLoc]=NULL;
        sortedSubstituteMetric[childIter]= substituteMetric[largestChildLoc];
        sortedSubstituteMetric2[childIter]= substituteMetric2[largestChildLoc];
        substituteRightChild[childIter]=NULL_CA_ID;
      }
      sortedPotentialSubstitutes[childIter]=largestChild;
      sortedSubstituteRightChild[childIter]=largestChildId;
    }

    // following four arrays are used during insertion into memo
    RelExpr** resultPotentialSubstitutes =  sortedPotentialSubstitutes;
    CostScalar* resultSubstituteMetric = sortedSubstituteMetric;
    CostScalar* resultSubstituteMetric2 = sortedSubstituteMetric2;
    CANodeId* resultSubstituteRightChild = sortedSubstituteRightChild;

    // Sort based on dataflow - End

    // Insert into substitute memory - Begin

    // number of substitute remaining after dataflow based pruning
    numSubstitutes-=numPrunedSubstitutes;

    // Now we insert the winning potential Substitutes
    Int32 numGeneratedSubstitutes = 0;

    Int32 maxSubstitutes = numSubstitutes;

    maxSubstitutes = (ActiveSchemaDB()->getDefaults()).\
                       getAsLong(COMP_INT_51);

    if ((maxSubstitutes <= 0) ||
        (maxSubstitutes > numSubstitutes) ||
        (numChildren == 2))
      maxSubstitutes = numSubstitutes;

    Int32 potential = maxSubstitutes - 1;

    // If this is the top multijoin
    if (before->getGroupAttr()->getPotential() < 0)
    {
      before->getGroupAttr()->updatePotential(0);
    }

    Int32 groupPotential = before->getGroupAttr()->getPotential();

    Int32 combinedPotentialThreshold =
      CURRSTMT_OPTDEFAULTS->getEnumPotentialThreshold();

    for (childIter = (numSubstitutes - maxSubstitutes);
         childIter < numSubstitutes;
         childIter++)
    {
      RelExpr* substitute = resultPotentialSubstitutes[childIter];

#ifdef _DEBUG
        if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
             CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
        {
          CURRCONTEXT_OPTDEBUG->stream() << "For right child "
            << (CollIndex) resultSubstituteRightChild[childIter]
            << " " << resultSubstituteMetric[childIter].value()
            << " ~~ "<< minSubstituteMetric.value() << endl;
        }
#endif
      Int32 combinedPotential = groupPotential + potential;

      substitute->updatePotential(potential);
      if(substitute->child(0)->getOperatorType()==REL_MULTI_JOIN)
      {
        substitute->child(0)->getGroupAttr()
          ->updatePotential(combinedPotential);
      }
      else
      {
        // consider every alternative plan for the left most join of a JBB.
        combinedPotentialThreshold =
          (ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_24);
      }

      if(combinedPotential <= combinedPotentialThreshold)
      {
        memory->insert(substitute);
        numGeneratedSubstitutes++;
      }

      potential--;
    }
  //"potentialSubstitutes" will be freed by statementHeap
  //"<temporary>"  will be freed by statementHeap
  //"substituteMetric" will be freed by statementHeap
  //"substituteMetric2" will be freed by statementHeap
  //coverity[leaked_storage]

    // Insert into substitute memory - Begin

  }
  if (memory != NULL)
  {
    result = memory->getNextSubstitute();
    if (result == NULL)
    {
      // returned all the substitutes
      // now delete the substitute memory, so we won't be called again
      delete memory;
      memory = NULL;
      return result;
    }

    result->synthLogProp();
    //result->child(0)->synthLogProp();   already called above while preparing
    //                                    substitutes

    result->contextInsensRules() += GlobalRuleSet->transformationRules();
    result->contextInsensRules() -= JoinToTSJRuleNumber;
    result->contextInsensRules() -= RoutineJoinToTSJRuleNumber;
    // Also allow join commutativity if arity is > 2
    if (mjoin->getArity() > 2)
      result->contextInsensRules() -= JoinCommutativityRuleNumber;

    // return the next retrieved substitute
    return result;
  }

  return result;
}

// -----------------------------------------------------------------------
// methods for MJStarJoinIRule
// -----------------------------------------------------------------------

NABoolean MJStarJoinIRule::topMatch (RelExpr * expr,
                                         Context * context)
{
  if (CmpCommon::getDefault(COMP_BOOL_115) == DF_ON)
    return topMatch_Old(expr, context);

  // COMP_BOOL_2 determines if LargeScope rules are tried
  if (CmpCommon::getDefault(COMP_BOOL_2) == DF_ON)
    return FALSE;

  // MJ rules should only fire on a MultiJoin
  if (NOT expr->getOperator().match(REL_MULTI_JOIN))
    return FALSE;

  // Get a handle to the MultiJoin on which we are performing the topMatch()
  MultiJoin* mjoin = (MultiJoin*) expr;

  // The StarJoin Type-I rule fires only on the top MultiJoin in a JBB
  // and following recursive calls which are marked by the scheduledLSRs set.
  // This scheduledLSRs is set by the StarJoin type-I Rule itself when it produces
  // a substitute. If the substitute i.e. the plan resulting from the application
  // of the StarJoin Type-I rule has children that are MultiJoins, the rule schedules
  // the application of the StarJoin Type-I rule on those children via the
  // scheduledLSRs set.
  if((mjoin->getJBBSubset().getJBBCs() != mjoin->getJBBSubset().getJBB()->getJBBCs()) &&
     (!mjoin->scheduledLSRs().contains(MJStarJoinIRuleNumber)))
    return FALSE;

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinIRule_TopMatch_Begin" <<endl;
    CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinI Rule TopMatch on " << mjoin->getJBBSubset().getText() << endl;
    CURRCONTEXT_OPTDEBUG->stream() << "Superset is : " << mjoin->getJBBSubset().getJBB()->getMainJBBSubset().getText() << endl;
  }
#endif //_DEBUG

  // get a handle to the JBBSubset Analysis for the JBBSubset
  // represented by this MultiJoin (i.e. mjoin)
  JBBSubsetAnalysis * mjoinAnalysis = mjoin->
                                        getJBBSubset().
                                          getJBBSubsetAnalysis();

  mjoinAnalysis->analyzeInitialPlan();

  if (mjoinAnalysis->starJoinTypeIFeasible())
  {
#ifdef _DEBUG
    if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
         CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
    {
      CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinIRule is feasible" <<endl;
      CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinIRule_TopMatch_End" <<endl;
    }
#endif //_DEBUG
    return TRUE;
  }
  else {
    mjoin->scheduledLSRs() += MJStarJoinIIRuleNumber;
#ifdef _DEBUG
    if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
         CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
    {
      CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinIRule is not feasible" <<endl;
      CURRCONTEXT_OPTDEBUG->stream() << "Scheduled MJStarJoinIIRule" <<endl;
      CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinIRule_TopMatch_End" <<endl;
    }
#endif //_DEBUG
  }
  return FALSE;
}

// the old topMatch method of MJStarJoinIRule, 
// not used anymore this code is OFF by default
NABoolean MJStarJoinIRule::topMatch_Old (RelExpr * expr,
                                         Context * context)
{
  // COMP_BOOL_2 determines if LargeScope rules are tried
  if (CmpCommon::getDefault(COMP_BOOL_2) == DF_ON)
    return FALSE;

  // MJ rules should only fire on a MultiJoin
  if (NOT expr->getOperator().match(REL_MULTI_JOIN))
    return FALSE;

  // Get a handle to the MultiJoin on which we are performing the topMatch()
  MultiJoin* mjoin = (MultiJoin*) expr;

  // The StarJoin Type-I rule fires only on the top MultiJoin in a JBB
  // and following recursive calls which are marked by the scheduledLSRs set.
  // This scheduledLSRs is set by the StarJoin type-I Rule itself when it produces
  // a substitute. If the substitute i.e. the plan resulting from the application
  // of the StarJoin Type-I rule has children that are MultiJoins, the rule schedules
  // the application of the StarJoin Type-I rule on those children via the
  // scheduledLSRs set.
  if((mjoin->getJBBSubset().getJBBCs() != mjoin->getJBBSubset().getJBB()->getJBBCs()) &&
     (!mjoin->scheduledLSRs().contains(MJStarJoinIRuleNumber)))
    return FALSE;

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinIRule_TopMatch_Begin" <<endl;
    CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinI Rule TopMatch on " << mjoin->getJBBSubset().getText() << endl;
    CURRCONTEXT_OPTDEBUG->stream() << "Superset is : " << mjoin->getJBBSubset().getJBB()->getMainJBBSubset().getText() << endl;
  }
#endif //_DEBUG

  // create a new work area
  // we create a work area to save work done during the topMatch(),
  // so that the results can be re-used while producing the nextSubstitute()
  MJStarJoinIRuleWA * mjStarJoinIRuleWA = new MJStarJoinIRuleWA();

  // get a handle to the JBBSubset Analysis for the JBBSubset
  // represented by this MultiJoin (i.e. mjoin)
  JBBSubsetAnalysis * mjoinAnalysis = mjoin->
                                        getJBBSubset().
                                          getJBBSubsetAnalysis();

  // assert if there is already a MJStarJoinIRuleWA associated with this mjoin
  CMPASSERT(!mjoinAnalysis->getMJStarJoinIRuleWA());

  // get a handle to the MJRulesWA
  // This work area is used to store information that we compute
  // so that other interest MJRules can use it.
  MJRulesWA * mjRulesWA = mjoinAnalysis->getMJRulesWA();

  // set the StarJoin rule work area for this MultiJoin
  mjoinAnalysis->setMJStarJoinIRuleWA(mjStarJoinIRuleWA);

  // Cardinality of fact table after application of local predicates on prefix
  // of clustering key
  CostScalar factTableCKPrefixCardinality;

  CANodeId largestTable = NULL_CA_ID;

  // determine if there is a clear fact table
  CANodeId factTable = findFactTable(mjoin,
                                     factTableCKPrefixCardinality,
                                     largestTable);

  mjRulesWA->factTable_ = factTable;
  mjRulesWA->largestTable_ = largestTable;

  // if we did not find a fact table then look for the center table
  if (factTable == NULL_CA_ID)
  {

    CANodeId centerTable = mjRulesWA->computeCenterTable();
    CostScalar centerTableSize = mjRulesWA->centerTableDataScanned_;
    CostScalar centerTableSizePerPartition =
      mjRulesWA->centerTableDataPerPartition_;
    CostScalar centerTablePartitions = mjRulesWA->centerTablePartitions_;

    if(centerTable != NULL_CA_ID)
    {
      JBBC * centerTableJBBC = centerTable.getNodeAnalysis()->getJBBC();

      if(!(centerTableJBBC->getPredecessorJBBCs().entries()))
      {
        if(centerTable == largestTable)
          factTable = centerTable;
        else
        {
          if((centerTableSizePerPartition >
              ((ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_16)*1024))||
             (centerTablePartitions >
              (ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_17)))
          {
            factTable = centerTable;
          }
        }
      }
    }

    // if we did not find a center table
    if (factTable == NULL_CA_ID)
    {
      mjoin->scheduledLSRs() += MJStarJoinIIRuleNumber;

#ifdef _DEBUG
      if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
           CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
      {
        CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinIRule is not feasible" <<endl;
        CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinIRule_TopMatch_End" <<endl;
      }
#endif //_DEBUG

      // return FALSE, indicating this rule is not a good match for the current
      // query
      return FALSE;
    }
  }

  // set the factTable in the work area
  mjStarJoinIRuleWA->factTable_ = factTable;

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "FactTable: " << factTable.getText() << endl;
    CURRCONTEXT_OPTDEBUG->stream() << endl;
  }
#endif

  // check to see if the MultiJoin matches a star pattern
  if (!isAStarPattern(mjoin, factTable, factTableCKPrefixCardinality))
  {
    // if not, then schedule other LSRs
    mjoin->scheduledLSRs() += MJStarJoinIIRuleNumber;

#ifdef _DEBUG
    if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
         CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
    {
      CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinIRule is not feasible" <<endl;
      CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinIRule_TopMatch_End" <<endl;
    }
#endif //_DEBUG

    // return FALSE, indicating this rule is not a good match for the current
    // query
    return FALSE;
  }

  // matched the Star Schema Pattern, set flag to indicate that in the work area
  mjStarJoinIRuleWA->matchedStarSchema_ = TRUE;

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinIRule is feasible" <<endl;
    CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinIRule_TopMatch_End" <<endl;
  }
#endif //_DEBUG

  return TRUE;
}

RelExpr * MJStarJoinIRule::nextSubstitute(RelExpr * before,
                                          Context * context,
                                          RuleSubstituteMemory * &memory)
{
  if (CmpCommon::getDefault(COMP_BOOL_115) == DF_ON)
    return nextSubstitute_Old(before, context, memory);

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinIRule_nextSubstitute_Begin" <<endl;
  }
#endif //_DEBUG

  MultiJoin * mjoin = (MultiJoin *) before;

  // Make sure GroupAnalysis localJBBView is the same as mjoin's JBBSubset.
  CMPASSERT(mjoin->getJBBSubset() ==
            *(mjoin->getGroupAttr()->getGroupAnalysis()->getLocalJBBView()));

  // get a handle to the JBBSubset Analysis for the JBBSubset
  // represented by this MultiJoin (i.e. mjoin)
  JBBSubsetAnalysis * mjoinAnalysis = mjoin->
                                        getJBBSubset().
                                          getJBBSubsetAnalysis();

  const NAList<CANodeIdSet> * const leftDeepJoinSequence =
    mjoinAnalysis->getInitialPlanLeftDeepJoinSequence();

  UInt32 numEntriesInLeftDeepJoinSequence =
    (*leftDeepJoinSequence).entries();

  // setup the join directives for the left deep join sequence
  NAList<MJJoinDirective *> joinDirectives(CmpCommon::statementHeap(),
                                           numEntriesInLeftDeepJoinSequence-1);

  NABoolean encounteredFactTable = FALSE;

  UInt32 i = 0;

  CANodeId factTable = mjoinAnalysis->getFactTable();

  for(i=0;
      i < (numEntriesInLeftDeepJoinSequence-1);
      i++)
  {
    MJJoinDirective * joinDirective = new MJJoinDirective(CmpCommon::statementHeap());
    CANodeIdSet joinRightChild = (*leftDeepJoinSequence)[i];
    JBBC * joinRightChildJBBC = NULL;
    
    if(joinRightChild.entries() == 1)
      joinRightChildJBBC = joinRightChild.getFirst().getNodeAnalysis()->getJBBC();

    // setup unconditional join directives
    joinDirective->setSkipJoinLeftShift();

    // COMP_BOOL_7 is OFF my default therefore
    // this is essentially unconditional
    if (CmpCommon::getDefault(COMP_BOOL_7) == DF_OFF)
      joinDirective->setJoinFromPTRule();


    if(joinRightChild == factTable)
    {
      encounteredFactTable = TRUE;
      if (CmpCommon::getDefault(COMP_BOOL_71) == DF_OFF)
        joinDirective->setSkipHashJoin();
      joinDirective->setSkipMergeJoin();
      joinDirective->setJoinSource(Join::STAR_FACT);

    }
    else if (encounteredFactTable)
    {
      if(CmpCommon::getDefault(COMP_BOOL_100) == DF_OFF)
        joinDirective->setSkipMergeJoin();
      joinDirective->setJoinSource(Join::STAR_KEY_JOIN);
    }
    else {

      joinDirective->setJoinSource(Join::STAR_FILTER_JOIN);
      if ((CmpCommon::getDefault(COMP_BOOL_85) == DF_OFF) &&
          joinRightChildJBBC &&
          !(joinRightChildJBBC->getOriginalParentJoin() &&
            joinRightChildJBBC->getOriginalParentJoin()->isRoutineJoin()))
        joinDirective->setSkipNestedJoin();
    }

    // Turn OFF join commutativity if either is TRUE:
    // 1. If this is the NJ into the fact table
    // 2. If the join is before the NJ into the fact table and COMP_BOOL_5 = OFF
    // 3. If COMP_BOOL_3 = ON
    if (((joinRightChild == factTable) && encounteredFactTable) ||
        (encounteredFactTable && (CmpCommon::getDefault(COMP_BOOL_5) == DF_OFF)) ||
        (CmpCommon::getDefault(COMP_BOOL_3) == DF_ON))
    {
      joinDirective->setSkipJoinCommutativity();
    }

    if(joinRightChild.entries() > 1)
    {
      joinDirective->scheduleLSROnRightChild(MJStarJoinIRuleNumber);
    }

    // any special handling of the last join (i.e. last from the top)
    if(i == numEntriesInLeftDeepJoinSequence - 2)
    {
      CANodeIdSet joinLeftChild = (*leftDeepJoinSequence)[i+1];

      if(joinLeftChild.entries() > 1)
      {
        joinDirective->scheduleLSROnLeftChild(MJStarJoinIRuleNumber);
      }
    }

    joinDirectives.insert(joinDirective);
  }

  RelExpr * result = mjoin->createLeftLinearJoinTree(leftDeepJoinSequence,
                                                     &joinDirectives);

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinIRule_nextSubstitute_End" <<endl;
  }
#endif //_DEBUG

  return result;
}

// This is old code, most of this work in now done during
// analysis. This functionality was moved to the analysis
// phase as part of Adaptive Segmentation. Since we need
// to get an early heuristic plan to determine a reasonable
// degree of parallelism
RelExpr * MJStarJoinIRule::nextSubstitute_Old(RelExpr * before,
                                              Context * context,
                                              RuleSubstituteMemory * &memory)
{

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinIRule_nextSubstitute_Begin" <<endl;
  }
#endif //_DEBUG

  MultiJoin * mjoin = (MultiJoin *) before;

  // get a handle to the JBBSubset Analysis for the JBBSubset
  // represented by this MultiJoin (i.e. mjoin)
  JBBSubsetAnalysis * mjoinAnalysis = mjoin->
                                        getJBBSubset().
                                          getJBBSubsetAnalysis();

  // Make sure GroupAnalysis localJBBView is the same as mjoin's JBBSubset.
  CMPASSERT(mjoin->getJBBSubset() ==
            *(mjoin->getGroupAttr()->getGroupAnalysis()->getLocalJBBView()));

  //Get a handle to ASM
  AppliedStatMan * appStatMan = QueryAnalysis::ASM();

  MJStarJoinIRuleWA * mjStarJoinIRuleWA = mjoin->
                                          getJBBSubset().
                                            getJBBSubsetAnalysis()->
                                              getMJStarJoinIRuleWA();
  // create the plan
  // By this time the topMatch should have already done the following:
  // 1. figured out the fact table - FT
  // 2. matched the star schema pattern
  // 3. computed the list of edges to be joined before the fact table - BFT
  // 4. the list of tables that should be join after the fact table - AFT
  //
  // The BFT will be joined in the order the edges are found in
  // mjStarJoinIRuleWA->listOfEdges
  //
  // Following is how the join tree should look like
  //
  //               Join
  //               /  \
  //              /  AFT
  //             /
  //         Nested_Join
  //          /     \
  //        BFT     FT
  //
  // In this method we need to figure out the ordering of the tables in the AFT
  // and then produce a plan

  CANodeId factTable = mjStarJoinIRuleWA->factTable_;

  // begin - try to figure out the ordering of the tables in AFT

  // get the set of tables (BFT + FT).
  // we already know the join ordering for this set
  CANodeIdSet factTableSet = mjStarJoinIRuleWA->nodesJoinedBeforeFactTable_;
  factTableSet.insert(factTable);

  // get the set of table (AFT).
  // we don't know the join ordering of these tables except the fact that
  // they should be join after the fact table
  CANodeIdSet availableNodes = mjStarJoinIRuleWA->availableNodes_;

#ifdef _DEBUG
    //print the right child of the current join
    if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
    {

      CURRCONTEXT_OPTDEBUG->stream() << "Table to be joined after fact table "\
                      << availableNodes.getText() << endl;
      CURRCONTEXT_OPTDEBUG->stream() << "Fact table "\
                      << factTable.getText() << endl;
      CURRCONTEXT_OPTDEBUG->stream() << "Table to be joined before fact table "\
                      << mjStarJoinIRuleWA->nodesJoinedBeforeFactTable_.getText() << endl;
    }
#endif

  // this list should contain the available nodes in the order
  // they should be joined
  NAList<CANodeIdSet> availableNodesOrdered(CmpCommon::statementHeap(),
                                            availableNodes.entries());

  while (availableNodes.entries())
  {
    // Node that produces the fewest rows when joined to the factTableSet
    CANodeId smallestNode = NULL_CA_ID;

    CostScalar lowestCardinality = 0;

    for( CANodeId availableNode = availableNodes.init();
         availableNodes.next(availableNode);
         availableNodes.advance(availableNode))
    {
      JBBC * availableNodeJBBC =
        availableNode.getNodeAnalysis()->getJBBC();

      CANodeIdSet jBBCsAvailableNodeDependsOn =
        availableNodeJBBC->getPredecessorJBBCs();

      // make sure all JBBCs availableNode depends
      // on come before the available node
      if(!factTableSet.contains(jBBCsAvailableNodeDependsOn))
        continue;

      //get cardinality of joining factTableset to availableNode
      //to nonAvailableNodes
      CostScalar joinCardinality =
        appStatMan->
          joinJBBChildren(factTableSet, availableNode)->
            getResultCardinality();

      if((smallestNode == NULL_CA_ID) ||
         (joinCardinality < lowestCardinality))
      {
	lowestCardinality = joinCardinality;
	smallestNode = availableNode;
      }

    }

    // remove the smallest node from the available nodes
    availableNodes.remove(smallestNode);
    // add the smallest node to ordered list of available nodes
    availableNodesOrdered.insert(smallestNode);
    // update the factTableSet for the next iteration
    factTableSet.insert(smallestNode);
  }

  // end - try to figure out the ordering of the tables in AFT

  // begin - construct left deep join sequence

  // The list CANodeIdSets representing the join
  // sequence produced by this rule
  // Element 0: is inner most / top most element in left deep join sequence
  // Element 1: is left most/outer most element in left deep join sequence
  NAList<CANodeIdSet> leftDeepJoinSequence (CmpCommon::statementHeap(),
                                        mjoin->getJBBSubset().getJBBCs().entries());

  UInt32 i = 0;

  // first insert tables from AFT
  for(i = availableNodesOrdered.entries(); i > 0; i--)
  {
    leftDeepJoinSequence.insert(availableNodesOrdered[(i-1)]);
  }

  // insert the fact table
  leftDeepJoinSequence.insert(factTable);

  NAList<CANodeIdSet> * listOfEdges = mjStarJoinIRuleWA->listOfEdges_;

  // insert the tables from the BFT
  // tables in the BFT are organized as a list of edges
  for(i = mjStarJoinIRuleWA->optimalFTLocation_; i > 0; i--)
  {
    leftDeepJoinSequence.insert((*listOfEdges)[(i-1)]);
  }

  // Consolidate the fringes
  mjoinAnalysis->consolidateFringes(factTable, leftDeepJoinSequence);

  // end - construct left deep join sequence

  // setup the join directives for the left deep join sequence
  NAList<MJJoinDirective *> joinDirectives(CmpCommon::statementHeap(),leftDeepJoinSequence.entries()-1);

  UInt32 numEntriesInLeftDeepJoinSequence = leftDeepJoinSequence.entries();

  NABoolean encounteredFactTable = FALSE;

  for(i=0;
      i < (numEntriesInLeftDeepJoinSequence-1);
      i++)
  {
    MJJoinDirective * joinDirective = new MJJoinDirective(CmpCommon::statementHeap());
    CANodeIdSet joinRightChild = leftDeepJoinSequence[i];
    
    JBBC * joinRightChildJBBC = NULL;
        
    if(joinRightChild.entries() == 1)
      joinRightChildJBBC = joinRightChild.getFirst().getNodeAnalysis()->getJBBC();

    // setup unconditional join directives
    joinDirective->setSkipJoinLeftShift();

    // COMP_BOOL_7 is OFF my default therefore
    // this is essentially unconditional
    if (CmpCommon::getDefault(COMP_BOOL_7) == DF_OFF)
      joinDirective->setJoinFromPTRule();


    if(joinRightChild == factTable)
    {
      encounteredFactTable = TRUE;
      if (CmpCommon::getDefault(COMP_BOOL_71) == DF_OFF)
        joinDirective->setSkipHashJoin();
      joinDirective->setSkipMergeJoin();
      joinDirective->setJoinSource(Join::STAR_FACT);

    }
    else if (encounteredFactTable)
    {
      if(CmpCommon::getDefault(COMP_BOOL_100) == DF_OFF)
        joinDirective->setSkipMergeJoin();
      joinDirective->setJoinSource(Join::STAR_KEY_JOIN);
    }
    else {

      joinDirective->setJoinSource(Join::STAR_FILTER_JOIN);
      if ((CmpCommon::getDefault(COMP_BOOL_85) == DF_OFF) &&
          joinRightChildJBBC &&
          !(joinRightChildJBBC->getOriginalParentJoin() &&
            joinRightChildJBBC->getOriginalParentJoin()->isRoutineJoin()))
        joinDirective->setSkipNestedJoin();
    }

    if ((encounteredFactTable)||
        (CmpCommon::getDefault(COMP_BOOL_3) == DF_ON))
    {
      joinDirective->setSkipJoinCommutativity();
    }

    if(joinRightChild.entries() > 1)
    {
      joinDirective->scheduleLSROnRightChild(MJStarJoinIRuleNumber);
    }

    // any special handling of the last join (i.e. last from the top)
    if(i == numEntriesInLeftDeepJoinSequence - 2)
    {
      CANodeIdSet joinLeftChild = leftDeepJoinSequence[i+1];

      if(joinLeftChild.entries() > 1)
      {
        joinDirective->scheduleLSROnLeftChild(MJStarJoinIRuleNumber);
      }
    }

    joinDirectives.insert(joinDirective);
  }

  RelExpr * result = mjoin->createLeftLinearJoinTree(&leftDeepJoinSequence,
                                                     &joinDirectives);

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinIRule_nextSubstitute_End" <<endl;
  }
#endif //_DEBUG

  return result;
}

CANodeId MJStarJoinIRule::isAStarShape(MultiJoin * mjoin)
{
  //get CANodeIdSet representing this MultiJoin
  CANodeIdSet childSet = mjoin->getJBBSubset().getJBBCs();

  UInt32 n = childSet.entries();

  //For a multijoin to be a star shape given n children
  //(n-1) children should be connect to one other child
  //where as 1 child will be connected to n-1 children


  CANodeId center = NULL_CA_ID;

  // require atleast 3 tables
  if (n < 3)
    return NULL_CA_ID;

  for(CANodeId currentTable = childSet.init();
      childSet.next(currentTable);
      childSet.advance(currentTable))
  {
    UInt32 numConnectedChildren =
      currentTable.getNodeAnalysis()->getJBBC()->getJoinedJBBCs().entries();

    if ((center == NULL_CA_ID) && (numConnectedChildren == (n-1)))
      center = currentTable;
    else{
      if ( numConnectedChildren != 1)
      {
        return NULL_CA_ID;
      }
    }

  }

  return center;
}

// This method, given a fact table, matches a multi-join to a star pattern
// This method is called by the MJStarJoinIRule::topMatch()
NABoolean MJStarJoinIRule::isAStarPattern(MultiJoin * mjoin, CANodeId factTable, CostScalar factTableCKPrefixCardinality)
{

  if (CmpCommon::getDefault(COMP_BOOL_88) == DF_ON)
    return FALSE;

  if (NOT CURRSTMT_OPTDEFAULTS->isNestedJoinConsidered())
    return FALSE;

  // This method is called while doing the topMatch on a multi-join to
  // determine if it matches a star schema pattern.11

  // Get a handle to the work area
  // We need a work area to save work done while doing the
  // topMatch, since this work is re-used in the nextSubstitute.
  // This saves us from doing this heavy work again.
  MJStarJoinIRuleWA * mjStarJoinIRuleWA = mjoin->
                                          getJBBSubset().
                                            getJBBSubsetAnalysis()->
                                              getMJStarJoinIRuleWA();

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinIRule_isAStarPattern_Begin" <<endl;
  }
#endif //_DEBUG

  //get CANodeIdSet representing this MultiJoin
  CANodeIdSet childSet = mjoin->getJBBSubset().getJBBCs();

  //Get a handle to ASM
  AppliedStatMan * appStatMan = QueryAnalysis::ASM();

  //all nodes in this multi-join except the fact table
  CANodeIdSet nodesExcludingFactTable = childSet;
  nodesExcludingFactTable.remove(factTable);

  //temp vars used for iteration
  ULng32 i=0;
  CANodeId currentNode;
  CANodeIdSet currentNodeSet;
  CostScalar currentCardinality;

  // get the index_desc for the facttable primary index
  const IndexDesc * factTableIndexDesc =
    factTable.getNodeAnalysis()
      ->getTableAnalysis()
        ->getTableDesc()
          ->getClusteringIndex();

  const PartitioningFunction * factTablePartFunc =
    factTableIndexDesc

      ->getPartitioningFunction();

  ValueIdSet factTablePartKey;

  Int32 factTableNumPartitions = 1;

  NABoolean factIsHashPartitioned = FALSE;

  if (factTablePartFunc)
  {
    factTablePartKey =
      factTablePartFunc->getPartitioningKey();

    factTableNumPartitions =
      factTablePartFunc->getCountOfPartitions();

    factIsHashPartitioned =
      factTablePartFunc->isATableHashPartitioningFunction();

  }

  //for further processing we'll need the clustering key of the fact table
  ValueIdList factTableCK =
    factTableIndexDesc->getIndexKey();

  // Variables used during iteration
  ValueId keyColumn;
  CANodeIdSet connectedTables;
  CANodeIdSet tablesConnectedToColumn;
  CANodeIdSet tablesConnectedViaThisColumn;
  Lng32 numPrefixColsCoveredFromFactTableCK = 0;

  // this variable is used to  get the list of
  // fact table clustering key prefix columns
  // that are covered by join or local preds
  ValueIdList coveredFactTableCKPrefix;

  // temp vars to pass into getJBBCsConnectedToPrefixOfList()
  ValueIdSet dummyForJoinPreds;
  ValueIdSet dummyForLocalPreds;

  // get:
  // 1. Tables connected to Fact Table via join predicates on clustering key prefix
  // 2. Fact Table clustering key prefix columns that have a predicate on them
  // 3. Number of fact table clustering key prefix columns covered
  connectedTables = factTable.getNodeAnalysis()->
                              getTableAnalysis()->
                              getJBBCsConnectedToPrefixOfList(
				              nodesExcludingFactTable,
	                                      factTableCK,
	                                      numPrefixColsCoveredFromFactTableCK,
	                                      dummyForJoinPreds,
                                              dummyForLocalPreds);

  // if a prefix of the fact table clustering key is not covered
  // by a join predicate, then return FALSE
  if((!connectedTables.entries()))
  {
#ifdef _DEBUG
    if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
         CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
    {
      CURRCONTEXT_OPTDEBUG->stream() << "Pattern Not Matched, there is no join predicate on prefix of clustering key" <<endl;
      CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinIRule_isAStarPattern_End" <<endl;
    }
#endif //_DEBUG
    // return FALSE, indicating this rule is not a good match for the current
    // query
    return FALSE;
  }


  // this set will be used to keep track of tables
  // that are not part of the edges of the star
  // schema
  CANodeIdSet availableNodes(childSet);

  // remove fact table from available nodes
  availableNodes-=factTable;

  // remove tables directly connected to the fact table
  // via join predicates on fact table clustering key
  // prefix
  availableNodes-=connectedTables;

  // set of tables representing the edges of the star from tables connected to the
  // prefix of the fact table clustering key
  // start by building an edge from the tables connected to the first prefix column
  // then add to that the edge from the tables connected to the next prefix column
  // and so on....
  // once an edge is built it will be inserted in a list of edges (variable
  // listOfEdges) The list as evident from the comments above will be ordered by
  // the position of the fact table clustering key column through which the edge
  // is connected.
  CANodeIdSet edge;

  // variable used while iterating in the loops below
  CANodeIdSet currentEdge;

  // Set of all tables in edges that should be joined before fact table
  // i.e. rows coming out of this set of tables will be nested joined
  // into fact table
  // If this set is empty then the star pattern was not matched
  CANodeIdSet optimalEdgeSet;

  // Cardinality of the set of tables in an edge
  CostScalar dataFlowFromEdge;

  // Cardinality of the fact table after application of clustering key
  // prefix columns after joining with the tables in the set 'edge'.
  CostScalar factTableRowsToScan;

  // Rows returned from the factTable
  CostScalar dataFlowFromFactTable;

  // cost of accessing the fact Table as outer most table
  CostScalar factTableHashJoinCost;

  // compute the cost of accessing the fact Table as the outer most table
  factTableHashJoinCost = computeCostForFactTable(csZero,
                                                  factTableCKPrefixCardinality,
                                                  factTable,
                                                  mjoin);

  // variable will be used while iterating through the loops below
  // cost of nested join of fact table with tables from edge
  CostScalar factTableCost;

  // variable use to keep track of the lowest cost of a nested join
  // on the fact table
  //CostScalar lowestFactTableCost(csZero);
  CostScalar lowestCombinedCost(csZero);

  // This determines how much cheaper a fact table nested should be
  // vs hash join / full scan of the fact table, for a plan with a
  // nested join on the fact table to be considered feasible.
  // Currently the default value is 10, this can be adjusted after
  // further testing.
  // Remember we are trying to find if a type-I plan is feasible
  // If not we'll try a type-II plan (i.e. a plan with a hash
  // join on the fact table)
  UInt32 factTableCostFactor =
    (ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_3);

  // list of edges, where each edge starts from a column connected
  // to a key prefix of the fact table. The list is in the order
  // of the columns in the key of the fact table
  NAList<CANodeIdSet> * listOfEdges =
    new (CmpCommon::statementHeap()) NAList<CANodeIdSet> (CmpCommon::statementHeap(),
                                                          connectedTables.entries());


#ifdef _DEBUG
      if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
           CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
      {
        CURRCONTEXT_OPTDEBUG->stream() << "Cost estimate of fact table access as outer most table: "\
                        << istring(Lng32(factTableHashJoinCost.value()))\
                        <<endl;
        CURRCONTEXT_OPTDEBUG->stream() << "Star Join will make sense if fact table nested join access is "\
                        << istring(Lng32(factTableCostFactor))\
                        << " times cheaper" << endl;
      }
#endif //_DEBUG

  //Doing a nested join on the fact table will only make sense if accessing
  //fact table as outer most table is factTableCostFactor times as expensive
  //as doing a nested join
  factTableHashJoinCost = factTableHashJoinCost / factTableCostFactor;

  //set of connected tables that have already been used to build edges
  CANodeIdSet usedConnectedTables;

  UInt32 lookAhead = (ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_18);

  // in the loop below we iterate over the columns of the fact table clustering
  // key prefix that are covered by either local or join predicates.
  // We iterate in order of the column's position in the clustering key.
  // In each iteration we try to see if the column has a join pred then
  // start an edge from the table connected via the column.
  // This is done so that we get a list of edges in an order which would
  // allow for ordered probes into the nested join on the fact table
  // e.g. consider the following connectivity graph
  // fact table cluster key is : a,b,c in that order
  //
  //
  //          DIM2
  //           |
  //           b
  //           |
  //   DIM1-a-FACT-c-DIM3
  //
  //
  // Then we want to get the edges in order starting from the outer most
  // i.e. DIM1, DIM2, DIM3
  //
  // Since we are attempting to get a plan like
  //
  //                   NJ
  //                  /  \
  //                /      \
  //              NJ      FACT
  //             /  \
  //           /      \
  //         NJ       DIM3
  //        /  \
  //      /      \
  //    DIM1     DIM2

  //iterate over the covered prefix of the clustering key
  for (i = 0 ; i < (ULng32) numPrefixColsCoveredFromFactTableCK; i++)
  {
    // Get the covered prefix on the fact table clustering key
    // covered prefix is constituted by columns that have either
    // a local or a join predicate on them.

    // get a handle to the key column
    keyColumn = factTableCK[i];

    //get a handle to the column analysis for this column
    ColAnalysis * keyColAnalysis = keyColumn.colAnalysis();

    // check if this column has join predicates on it
    if(!keyColAnalysis->getConnectedJBBCs().entries())
    {
      // if this does not have a join predicate on it
      // then it's covered by a local pred. Add it to
      // the list of key prefix columns covered.
      // It could also be because of a veg this column
      // does not have a join rather a local pred e.g.
      //
      // select count(*)
      // from
      //   fact, dim
      // where
      //   fact.a = dim.a and
      //   dim.a = 10;
      //
      // In this case 'fact.a = 10' is implied via a veg
      // and the join between fact and dim is essentially
      // a cross product after application of local preds
      coveredFactTableCKPrefix.insert(keyColumn);
      continue;
    }

    // Get tables from this multi-join (i.e. the multi-join on which we are
    // doing the top match) that are connected to the fact table via this
    // column

    // This will get all tables connected via this column
    tablesConnectedViaThisColumn = keyColAnalysis->getConnectedJBBCs();

    // This will narrow down the set of tables from all connected
    // tables to connected tables in this multi-join
    tablesConnectedViaThisColumn.intersectSet(connectedTables);

    //tablesConnectedViaThisColumn should have atleast one entry
    CCMPASSERT(tablesConnectedViaThisColumn.entries());

    if(!tablesConnectedViaThisColumn.entries())
    {
      return FALSE;
    }

    // insert this column into list of key prefix columns covered
    coveredFactTableCKPrefix.insert(keyColumn);

    //get the number of tables connected via this columns
    UInt32 numTablesConnectedViaThisColumn =
      tablesConnectedViaThisColumn.entries();

    // Remove tables that have already been considered in previous iterations
    tablesConnectedViaThisColumn.subtractSet(usedConnectedTables);

    // get connected tables that have not been considered
    // i.e. we have not built an edge starting from them.
    UInt32 connectedTablesNotConsidered =
      tablesConnectedViaThisColumn.entries();

    // there are no connected tables to consider
    if(!connectedTablesNotConsidered)
      continue;

    // if tables connected via this column is less than the number of
    // connected tables not considered
    if(numTablesConnectedViaThisColumn > connectedTablesNotConsidered)
    {
      // This would happen in the following scenario
      // consider fact table key is a, b, c
      // consider column connectivity as below
      //
      //           a     b    c
      //           |     |    |
      //          DIM1 DIM1   |
      //           |          |
      //          DIM2       DIM2
      //
      // Notice here when we come to b, we would have already covered b if
      // we started an edge from DIM1 while iterating for column 'a'. Therefore
      // b is already covered and we can skip over to column c.
      //
      // This is a special case but I was worried about the case where all dimension
      // table might joined to the fact table via a partitioning key column in addition
      // to what ever other part of the fact table key they join to.
      // This was apparently an MP practice to try to colocate joins in the same partitions
      //
      // In such a case it would be preferrable to get an ordering of DIM1,DIM2 rather
      // than DIM2, DIM1 for the probes going into the fact table nested join

      // This column is already covered, move to the next one
      continue;
    }

    // the next table to consider for starting an edge
    CANodeId tableToConsider = NULL_CA_ID;

    if (tablesConnectedViaThisColumn.entries()==1)
    {

      // get the jbbc in this set
      tableToConsider = tablesConnectedViaThisColumn.getFirst();

    }
    else{

      Lng32 maxPrefix = 0;
      CANodeIdSet coveredTables;

      // Find the table that gives the max key coverage. Consider the example
      // below:
      // consider fact table key is a, b, c
      // consider column connectivity as below
      //
      //           a     b    c
      //           |     |    |
      //          DIM1 DIM1   |
      //           |          |
      //          DIM2       DIM2
      //
      // When iterating for column 'a' we would like to start an edge
      // from table DIM1 rather than DIM2, since 'a' will cover a greater
      // key prefix.

      for(CANodeId currentTable = tablesConnectedViaThisColumn.init();
          tablesConnectedViaThisColumn.next(currentTable);
          tablesConnectedViaThisColumn.advance(currentTable))
      {
	Lng32 coveredPrefix = 0;

	// add currentTable to list of tables that we
	// have already determined cover key prefix
	// In each iteration:
	// 1. Add the current table to this list
	// 2. determine the key prefix covered
	// 3. Remove current table from this list
	// Eventually the table that provides the
	// greatest key prefix will be considered
	usedConnectedTables.insert(currentTable);

        // find the key prefix covered
        coveredTables = factTable.getNodeAnalysis()->
                                  getTableAnalysis()->
                                  getJBBCsConnectedToPrefixOfList(
                                    usedConnectedTables,
                                    factTableCK,
      	                            coveredPrefix,
      	                            dummyForJoinPreds,
                                    dummyForLocalPreds);

        // if covered prefix is the greater than the
        // max prefix we have till now, then consider
        // this table to start an edge from.
        if((tableToConsider == NULL_CA_ID) ||
           (maxPrefix < coveredPrefix))
        {
	  tableToConsider = currentTable;
	  maxPrefix = coveredPrefix;
	}

	// remove currentTable from list of tables that we
	// have already determined cover key prefix
	usedConnectedTables.remove(currentTable);

      }

    }

    // finally we have determined the table that provides for
    // the greatest key prefix on the fact table, add it to
    // the list of connected tables for which we had built edges
    usedConnectedTables.insert(tableToConsider);

    //build the edge
    currentEdge = extendEdge(tableToConsider, availableNodes,lookAhead);

    (*listOfEdges).insert(currentEdge);

    //edge += currentEdge;
#ifdef _DEBUG
    if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
         CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
    {
      CURRCONTEXT_OPTDEBUG->stream() << "Built an edge starting from table "\
                      << tableToConsider.getText()<< endl;
      CURRCONTEXT_OPTDEBUG->stream() << "The edge is " \
                      << currentEdge.getText()<<endl;
    }
#endif

  }

  // variable used in iteration
  CANodeId connectedTable = NULL_CA_ID;

  // if there are still some tables left that were on the key columns
  // but were not considered earlier because we where optimizing key
  // coverage
  if(usedConnectedTables.entries() != connectedTables.entries())
  {
    // build an edge from all the tables in the unusedConnectedTables set
    // since they still have predicates on fact table clustering key prefix
    // columns and could provide reduction.
    CANodeIdSet unusedConnectedTables(connectedTables);
    unusedConnectedTables.subtractSet(usedConnectedTables);

    for(connectedTable = unusedConnectedTables.init();
                         unusedConnectedTables.next(connectedTable);
                         unusedConnectedTables.advance(connectedTable))
    {

      //build the edge
      currentEdge = extendEdge(connectedTable, availableNodes, lookAhead);

      (*listOfEdges).insert(currentEdge);

       //edge += currentEdge;
#ifdef _DEBUG
      if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
           CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
      {
        CURRCONTEXT_OPTDEBUG->stream() << "Built an edge starting from table "\
                        << connectedTable.getText()<< endl;
        CURRCONTEXT_OPTDEBUG->stream() << "The edge is " \
                        << currentEdge.getText()<<endl;
      }
#endif
    }
  }


  // We have the list of edges joined to the fact table in the order
  // of the fact table clustering key columns.
  // Find the optimal location for the fact table, i.e. we'll try to
  // see which tables should be join below the fact table in a left
  // deep join sequence.
  //
  // e.g. consider the following connectivity graph
  // fact table cluster key is : a,b,c in that order
  //
  //
  //          DIM2
  //           |
  //           b
  //           |
  //   DIM1-a-FACT-c-DIM3
  //
  //
  // Then the list of edges will be DIM1, DIM2, DIM3
  //
  // In the following loop we'll estimate the cost of
  // doing a nested join on the fact table after
  // 1. DIM1,
  // 2. DIM1, DIM2
  // 3. DIM1, DIM2, DIM3
  //
  // Then we compare the optimal cost of doing a nested join
  // on the fact table, with the cost of just doing a hash
  // join on the fact table which basically involves scanning the
  // whole fact table.

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "Starting to evaluate optimal location of Fact Table "<< endl;

  }
#endif

  //cost of fact table nested join after
  //maximum possible key prefix coverage
  CostScalar costForMaxKeyCoverage=0;

  CostScalar costOfDimProbes(csZero);
  double probeHashTableUnitCost =
    (double) (ActiveSchemaDB()->getDefaults()).getAsDouble
    (MULTI_JOIN_PROBE_HASH_TABLE);

  // iterate over list of edges
  for ( i = 0; i < (*listOfEdges).entries(); i++)
  {

    // get the current edge from the list of edges
    currentEdge = (*listOfEdges)[i];

    //the edge that was used in the last iteration
    CANodeIdSet prevEdgeAndFact = edge;
    prevEdgeAndFact += factTable;

    // add it to the cumulative edge, i.e. list of
    // all tables we are considering to be below the
    // fact table
    edge += currentEdge;

    // get the number of rows coming out of all the tables
    // that'll be below the fact tables.
    // This is the number of probes going into the fact table
    // for this nested join
    dataFlowFromEdge = appStatMan->
                         getStatsForCANodeIdSet(edge)->
                           getResultCardinality();

    if((factIsHashPartitioned) &&
       (CmpCommon::getDefault(COMP_BOOL_150) == DF_OFF))
    {
      ValueIdSet factTableColumnsJoined = factTable.getNodeAnalysis()
                                            ->getTableAnalysis()
                                              ->getColsConnectedViaEquiJoinPreds(edge);

      CollIndex numPartKeyCols = factTablePartKey.entries();
      CollIndex numJoinedCols = factTableColumnsJoined.entries();
      CollIndex numPartKeyColsCovered = 0;

      for (ValueId VId = factTablePartKey.init();
           factTablePartKey.next(VId);
           factTablePartKey.advance(VId) )
      {
        ValueIdSet tempJoinedCols = factTableColumnsJoined;
        tempJoinedCols.removeCoveredVIdSet(VId);
        if(tempJoinedCols.entries() < numJoinedCols)
          numPartKeyColsCovered++;
        else
        break;
      }

      if(numPartKeyColsCovered < numPartKeyCols)
      {
        dataFlowFromEdge = dataFlowFromEdge * factTableNumPartitions;
      }

    }

    // figure out the number of fact table rows that'll be scanned for this
    // nested join
    factTableRowsToScan = appStatMan->
                            getStatsForJoinPredsOnCKOfJBBC(edge, factTable)->
                              getResultCardinality();

    // figure out the rows coming out of the fact table after this nested join
    // using joinJBBChildren(prevEdgeAndFact, currentEdge) instead of
    // using joinJBBChildren(edge, factTable), in order to drive ASM using
    // joins, 'edge' could have cross products and will miss some cardinality
    // adjustments because of join with a potential cross product.
    dataFlowFromFactTable = appStatMan->
                              joinJBBChildren(prevEdgeAndFact, currentEdge)->
                                getResultCardinality();

    // now cost the nested join
    factTableCost = computeCostForFactTable(dataFlowFromEdge,
                                            factTableRowsToScan,
                                            factTable,
                                            mjoin);

    // capture the cost of the nested join when all edges are join below the
    // fact table.
    // This will be the cost for max key coverage after the last iteration.
    costForMaxKeyCoverage = factTableCost;

#ifdef _DEBUG
    if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
         CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
    {
      CURRCONTEXT_OPTDEBUG->stream() << "FactTable after edge " \
                      << currentEdge.getText()<<endl;
      CURRCONTEXT_OPTDEBUG->stream() << "The cummulative edge is "\
                      << edge.getText()<<endl;
      CURRCONTEXT_OPTDEBUG->stream() << "Probes into fact table: "\
                      << istring(Lng32(dataFlowFromEdge.value()))\
                      <<endl;
      CURRCONTEXT_OPTDEBUG->stream() << "Fact Table Rows to scan: "\
                      << istring(Lng32(factTableRowsToScan.value()))\
                      <<endl;
      CURRCONTEXT_OPTDEBUG->stream() << "Rows coming out of fact table: "\
                      << istring(Lng32(dataFlowFromFactTable.value()))\
                      <<endl;
      CURRCONTEXT_OPTDEBUG->stream() << "Our cost estimate of fact table nested join: "\
                      << istring(Lng32(factTableCost.value()))\
                      <<endl;
    }
#endif //_DEBUG

    // if this is the first iteration
    // or current nested join cost is cheaper
    // than lowest nested join cost
    CostScalar combinedCost = costOfDimProbes + factTableCost;
    if ((NOT lowestCombinedCost.isGreaterThanZero()) ||
        (combinedCost < lowestCombinedCost))
    {
      lowestCombinedCost = combinedCost;

      if (lowestCombinedCost <= factTableHashJoinCost)
      {
        optimalEdgeSet = edge;

	// set the set Of tables to be joined before the fact table,
	// this will be used during nextSubstitute for this rule.
	mjStarJoinIRuleWA->nodesJoinedBeforeFactTable_ =
	  optimalEdgeSet;

        // set the optimal location of the fact table in the
        // list of edges, this will be used by the nextSubstitute()
        // method
        mjStarJoinIRuleWA->optimalFTLocation_ = (Int32) i+1;
      }
    }
#if 0
    if ((NOT lowestFactTableCost.isGreaterThanZero()) ||
        (factTableCost < lowestFactTableCost))
    {
      lowestFactTableCost = factTableCost;

      if (lowestFactTableCost <= factTableHashJoinCost)
      {
        optimalEdgeSet = edge;

	// set the set Of tables to be joined before the fact table,
	// this will be used during nextSubstitute for this rule.
	mjStarJoinIRuleWA->nodesJoinedBeforeFactTable_ =
	  optimalEdgeSet;

        // set the optimal location of the fact table in the
        // list of edges, this will be used by the nextSubstitute()
        // method
        mjStarJoinIRuleWA->optimalFTLocation_ = (Int32) i+1;
      }
    }
#endif
    costOfDimProbes += dataFlowFromEdge * probeHashTableUnitCost;
  }

  //This variable determines if we want all edges of the star
  //that join on the key prefix on the Fact Table, to be joined
  //below i.e. before the fact table.
  NABoolean forceMaxKeyCoverageOnFT = FALSE;

  // COMP_BOOL_12 is used to force a plan with max key coverage on
  // the fact table, i.e. join all edges covering key prefix column
  // below the fact table to get max key coverage for nested join
  // on fact table.

  // COMP_BOOL_11 is similar to COMP_BOOL_12. It differs to COMP_BOOL_12
  // in that it does not force a max key coverage plan if the cost of
  // the nested join is higher than the cost of the hash join on the
  // fact table.

  // COMP_BOOL_13 basically means to force a star join rule plan.

  //if we want to force max key prefix coverage on fact table
  //this should only be allowed if fact table nested join is
  //cheaper than doing a full scan of the fact table (i.e. the
  //hash join scenario)
  //If COMP_BOOL_12 is 'ON' then we'll just force a plan with
  //max key prefix coverage on fact table
  if ((costForMaxKeyCoverage < factTableHashJoinCost) &&
      (CmpCommon::getDefault(COMP_BOOL_11) == DF_ON))
    forceMaxKeyCoverageOnFT = TRUE;

  //If COMP_BOOL_12 is 'ON' then we'll just force a plan with
  //max key prefix coverage on fact table
  if (CmpCommon::getDefault(COMP_BOOL_12) == DF_ON)
    forceMaxKeyCoverageOnFT = TRUE;


  //COMP_BOOL_13 means force star join rule !!!
  //therefore if starjoin rule was not found feasible
  //just force max key coverage i.e. force all the
  //edges joining on FT key prefix columns to be below
  //fact table
  if (((!optimalEdgeSet.entries()) &&
       (CmpCommon::getDefault(COMP_BOOL_13) == DF_ON)) &&
      (CmpCommon::getDefault(COMP_BOOL_77) != DF_ON))
    forceMaxKeyCoverageOnFT = TRUE;


  // if we are forcing max key prefix coverage for the
  // fact table nested join.
  if(forceMaxKeyCoverageOnFT)
  {
    edge.clear();

    // iterate over list of edges
    for ( i = 0; i < (*listOfEdges).entries(); i++)
    {

      currentEdge = (*listOfEdges)[i];

      edge += currentEdge;
    }

    optimalEdgeSet = edge;

    // set the set Of tables to be joined before the fact table,
    // this will be used during nextSubstitute for this rule.
    mjStarJoinIRuleWA->nodesJoinedBeforeFactTable_ =
      optimalEdgeSet;

    mjStarJoinIRuleWA->optimalFTLocation_ = (Int32) (*listOfEdges).entries();
  }

  // from the set of tables in all the edges that join to the key prefix
  // take out the set of tables that compose the fringes that should be joined
  // before the fact table
  edge.subtractSet(optimalEdgeSet);

  // add the remaining tables to the available nodes set
  // these tables will be joined above/after the fact table
  // in the left deep join sequence
  availableNodes.addSet(edge);

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
      CURRCONTEXT_OPTDEBUG->stream() << "The tables below fact table"\
                      << optimalEdgeSet.getText()<<endl;
      CURRCONTEXT_OPTDEBUG->stream() << "The tables above fact table "\
                      << availableNodes.getText()<<endl;

  }
#endif //_DEBUG


  // nodes to be joined after Fact Table
  mjStarJoinIRuleWA->availableNodes_ = availableNodes;

  // save list of edges in the work area it'll be used when by nextSubstitute()
  mjStarJoinIRuleWA->listOfEdges_ = listOfEdges;

  // if optimalEdgeSet is empty it could mean
  // 1. there were not join predicates on fact table clustering key prefix
  // 2. there were join predicates on fact table clustering key prefix but
  //    nested join on fact table clustering is just too expensive.
  // In either case, a type-I nested join plan is not a good idea
  if (!optimalEdgeSet.entries())
  {

#ifdef _DEBUG
    if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
         CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
    {
      CURRCONTEXT_OPTDEBUG->stream() << "Pattern Not Matched, there is no significant reduction on fact table" <<endl;
      CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinIRule_isAStarPattern_End" <<endl;
    }
#endif //_DEBUG
    // return FALSE, indicating this rule is not a good match for the current
    // query
    return FALSE;
  }

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "Pattern Matched" <<endl;
    CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinIRule_isAStarPattern_End" <<endl;
  }
#endif //_DEBUG
  return TRUE;
}

void MJStarJoinIRule::sortMJJBBCsByCardAfterLocalKeyPrefixPred(NAList<CANodeId> &sortedJBBCs,const MultiJoin * const mjoin)
{

  //Get a handle to ASM
  AppliedStatMan * appStatMan = QueryAnalysis::ASM();

  //get CANodeIdSet representing this MultiJoin
  CANodeIdSet childSet = mjoin->getJBBSubset().getJBBCs();

  //The following loop performs the sorting of the JBBCs in
  //this multi-join and places the sorted list in sortedJBBCs
  for(Int32 i = 0; i < mjoin->getArity(); i++){

    //To start with, pick an arbitrary child as largest
    CANodeId largestChild = childSet.init();
    // cast to void to prevent Coverity CHECKED_RETURN error
    (void)childSet.next(largestChild);

    //get cardinality
    EstLogPropSharedPtr estLogProps = appStatMan->getStatsForLocalPredsOnCKPOfJBBC(largestChild);
    CostScalar largestCardinality = estLogProps->getResultCardinality();

    for( CANodeId child = largestChild; childSet.next(child); childSet.advance(child))
    {
      //get cardinality of child i.e. the size of the child
      estLogProps = appStatMan->getStatsForLocalPredsOnCKPOfJBBC(child);
      CostScalar childCardinality = estLogProps->getResultCardinality();

      //if this child's cardinality is larger than the largest child's
      //cardinality, set this child to be the largest
      if( childCardinality >= largestCardinality)
      {
        largestChild = child;
        largestCardinality=childCardinality;
      }
    }

    //remove the largest child from the child set
    childSet.remove(largestChild);
    //add largest child to sorted list
    sortedJBBCs.insert(largestChild);
  }

};//MJStarJoinIRule::sortMJJBBCsByCardAfterLocalKeyPrefixPred

CANodeId MJStarJoinIRule::findFactTable(const MultiJoin * const mjoin,
                                       CostScalar & factTableCKPrefixCardinality,
                                       CANodeId & biggestTable)
{
  //get CANodeIdSet representing this MultiJoin
  CANodeIdSet childSet = mjoin->getJBBSubset().getJBBCs();

  QueryAnalysis* qa = QueryAnalysis::Instance();
  JBBSubsetAnalysis* jbbSubsetAnalysis = 
    qa->findJBBSubsetAnalysis (mjoin->getJBBSubset());

  CANodeId factTable = jbbSubsetAnalysis->findFactTable 
    ( childSet, factTableCKPrefixCardinality, biggestTable );

  return factTable;

};//MJStarJoinIRule::findFactTable

//extends an edge of the star
CANodeIdSet MJStarJoinRules::extendEdge(CANodeId thisTable,//in
                                       CANodeIdSet& availableNodes,
                                       UInt32 lookAhead)//in\out
{
  // CANodeIdSet representing thisTable
  CANodeIdSet thisTableSet(thisTable);

  //Get a handle to ASM
  AppliedStatMan * appStatMan = QueryAnalysis::ASM();

  // return value
  CANodeIdSet edge;

  //remove this table from the availableNodes
  //if it there
  if(availableNodes.contains(thisTable))
    availableNodes.remove(thisTable);

  //extend the edge to include thisTable
  edge += thisTable;

  if(!availableNodes.entries())
    return edge;

  if(lookAhead)
    lookAhead -=1;

  //if(!lookAhead)
  //  return edge;

  // Tables connected to thisTable that are still available
  CANodeIdSet connectedTables;

  // get all connected tables that are still available
  // i.e. not already part of some edge.
  connectedTables = thisTable.getNodeAnalysis()
                      ->getJBBC()->getJoinedJBBCs();
  connectedTables.intersectSet(availableNodes);

  CANodeIdSet connectedTablesBackup = connectedTables;

  UInt32 numConnectedTables = connectedTables.entries();

  // get after local predicate cardinality of thisTable
  CostScalar thisTableCardinality =  appStatMan->
                                       getStatsForCANodeId(thisTable)->
                                         getResultCardinality();

  //temp var to capture the set representing edge extension
  CANodeIdSet edgeExtension;

  //temp var to get cardinality of edge as it is being built
  CostScalar currentEdgeCardinality;

  //variables used for iteration
  UInt32 i;
  CANodeId connectedTable;
  CANodeIdSet connectedTableSet;

  const CostScalar EXTENSION_ALLOWANCE(1.05); // We allow 5% increase

  // sort connected Tables by cardinality of join with thisTable
  for (i = 0;i < numConnectedTables; i++)
  {
    CostScalar lowestCardinality = 0;

    // A table from the set of connectedTables that produces the lowest
    // cardinality when joined to thisTable
    CANodeId smallestRemainingTable = NULL_CA_ID;

    for(connectedTable = connectedTables.init();
                         connectedTables.next(connectedTable);
                         connectedTables.advance(connectedTable))
    {

      //get a CANodeIdSet only containing the connectedTable to estimate
      //join of connectedTable with thisTable.
      connectedTableSet = connectedTable;

      //get cardinality for join of thisTable and connectedTable
      CostScalar joinCardinality =
        appStatMan->joinJBBChildren(thisTableSet, connectedTableSet)
          ->getResultCardinality();

      if(smallestRemainingTable == NULL_CA_ID)
      {
        lowestCardinality = joinCardinality;
        smallestRemainingTable = connectedTable;
        continue;
      }

      if(joinCardinality < lowestCardinality)
      {
        lowestCardinality = joinCardinality;
        smallestRemainingTable = connectedTable;
      }
    }

    //add the table that produces the lowest cardinality
    //after joining with thisTable
    //connectedTablesSorted.insert(smallestRemainingTable);
    if(smallestRemainingTable == NULL_CA_ID)
      continue;

    JBBC * smallestRemainingTableJBBC =
      smallestRemainingTable.getNodeAnalysis()->getJBBC();

    JBBSubset * thisTableJBBSubset =
      thisTableSet.jbbcsToJBBSubset();

    NABoolean isGuaranteedNonExpandingJoin =
      thisTableJBBSubset->
        isGuaranteedNonExpandingJoin(*smallestRemainingTableJBBC);

    // if the smallestRemaingTable
    if((lowestCardinality <= (EXTENSION_ALLOWANCE * thisTableCardinality))&&
       (isGuaranteedNonExpandingJoin ||
        lookAhead ||
        smallestRemainingTableJBBC->isOneRowMax()))
    {
      //sortedListOfConnectedReducingTables.insert(connectedTable);
      //only extend to a table that is still available
      if(availableNodes.contains(smallestRemainingTable))
      {
	edgeExtension = extendEdge(smallestRemainingTable, availableNodes, lookAhead);

	// get cardinality for join of edge and edgeExtension
	// I don't really need to do this, but doing this to
	// better drive the ASM cache
	currentEdgeCardinality =
	  appStatMan->joinJBBChildren(edge, edgeExtension)
            ->getResultCardinality();

        edge += edgeExtension;
      }
    }

    //remove it from connectedTables
    connectedTables.remove(smallestRemainingTable);

  }

  return edge;
}
//MJStarJoinRules::extendEdge

// get a rough estimate of cost for doing a nested join on the fact table
// number of probes = dataFlowFromEdge
// Rows of fact table that will be scanned = factTableRowsToScan
CostScalar MJStarJoinIRule::computeCostForFactTable(CostScalar probes,
                                                   CostScalar factTableRowsToScan,
                                                   CANodeId   factTable,
                                                   MultiJoin  * mjoin)
{
  //Get a handle to ASM
  AppliedStatMan * appStatMan = QueryAnalysis::ASM();

  CostScalar costPerProbe ((ActiveSchemaDB()->getDefaults()).getAsDouble(COMP_FLOAT_0));
  CostScalar costPerUnitSize ((ActiveSchemaDB()->getDefaults()).getAsDouble(COMP_FLOAT_1));
  CostScalar costPerRowReturned ((ActiveSchemaDB()->getDefaults()).getAsDouble(COMP_FLOAT_2));

  //Record Size of the factTable
  CostScalar factTableRecordSize = factTable.getNodeAnalysis()->
                                             getTableAnalysis()->
                                             getTableDesc()->
                                             getNATable()->
                                             getRecordLength();

  CostScalar subsetSizeKB = (factTableRowsToScan * factTableRecordSize)/1024;

  CostScalar factTableRowsReturned = appStatMan->
                                     getStatsForCANodeId(factTable)->
                                     getResultCardinality();

  ExprGroupId factTableExprGroupId = mjoin->getChildFromJBBCId(factTable);

  CostScalar sizeOfRowReturnedFromFactTable =
    factTableExprGroupId.getGroupAttr()->getRecordLength();


  CostScalar costForFactTable = (costPerProbe * probes) +
                                (costPerUnitSize * subsetSizeKB) +
                                (costPerRowReturned *
                                 factTableRowsReturned *
                                 sizeOfRowReturnedFromFactTable);

  return costForFactTable;
}

// sort list of JBBCs in descending order i.e.
// * first element is largest
// * last element is smallest
// the after local predicate cardinality of a JBBC is used for ordering
void MJStarJoinIRule::sortMJJBBCs(NAList<CANodeId> &sortedJBBCs,const MultiJoin * const mjoin)
{

  //Get a handle to ASM
  AppliedStatMan * appStatMan = QueryAnalysis::ASM();

  //get CANodeIdSet representing this MultiJoin
  CANodeIdSet childSet = mjoin->getJBBSubset().getJBBCs();

  //The following loop performs the sorting of the JBBCs in
  //this multi-join and places the sorted list in sortedJBBCs
  for(Int32 i = 0; i < mjoin->getArity(); i++){

    //To start with, pick an arbitrary child as largest
    CANodeId largestChild = childSet.init();
    // cast to void to prevent Coverity CHECKED_RETURN error
    (void)childSet.next(largestChild);

    //get cardinality
    EstLogPropSharedPtr estLogProps = appStatMan->getStatsForCANodeId(largestChild);
    CostScalar largestCardinality = estLogProps->getResultCardinality();

    for( CANodeId child = largestChild; childSet.next(child); childSet.advance(child))
    {
      //get cardinality of child i.e. the size of the child
      estLogProps = appStatMan->getStatsForCANodeId(child);
      CostScalar childCardinality = estLogProps->getResultCardinality();

      //if this child's cardinality is larger than the largest child's
      //cardinality, set this child to be the largest
      if( childCardinality >= largestCardinality)
      {
        largestChild = child;
        largestCardinality=childCardinality;
      }
    }

    //remove the largest child from the child set
    childSet.remove(largestChild);
    //add largest child to sorted list
    sortedJBBCs.insert(largestChild);
  }

};//MJStarJoinIRule::sortMJJBBCs

// -----------------------------------------------------------------------
// methods for MJStarJoinIIRule
// -----------------------------------------------------------------------

NABoolean MJStarJoinIIRule::topMatch (RelExpr * expr,
                                    Context * context)
{
  // COMP_BOOL_2 determines if LargeScope rules are tried
  if (CmpCommon::getDefault(COMP_BOOL_2) == DF_ON)
    return FALSE;

  // MJ rules should only fire on a MultiJoin
  if (NOT expr->getOperator().match(REL_MULTI_JOIN))
    return FALSE;

  // Get a handle to the MultiJoin on which we are performing the topMatch()
  MultiJoin* mjoin = (MultiJoin*) expr;

  // The StarJoin Type-II rule fires if it has been scheduled
  // The rule schedule StarJoin Type-I on any MultiJoins present
  // in the tree it produces
  if(!mjoin->scheduledLSRs().contains(MJStarJoinIIRuleNumber))
    return FALSE;

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinIIRule_TopMatch_Begin" <<endl;
    CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinIIRule TopMatch on " << mjoin->getJBBSubset().getText() << endl;
    CURRCONTEXT_OPTDEBUG->stream() << "Superset is : " << mjoin->getJBBSubset().getJBB()->getMainJBBSubset().getText() << endl;
  }
#endif //_DEBUG

  // get a handle to the JBBSubset Analysis for the JBBSubset
  // represented by this MultiJoin (i.e. mjoin)
  JBBSubsetAnalysis * mjoinAnalysis = mjoin->
                                        getJBBSubset().
                                          getJBBSubsetAnalysis();

  mjoinAnalysis->analyzeInitialPlan();

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinIIRule_TopMatch_End" <<endl;
  }
#endif //_DEBUG

  return TRUE;
}

RelExpr * MJStarJoinIIRule::nextSubstitute(RelExpr * before,
                                           Context * context,
                                           RuleSubstituteMemory * &memory)
{
  if (CmpCommon::getDefault(COMP_BOOL_115) == DF_ON)
    return nextSubstitute_Old(before, context, memory);

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinIIRule_nextSubstitute_Begin" <<endl;
  }
#endif //_DEBUG

  MultiJoin * mjoin = (MultiJoin *) before;

  // Make sure GroupAnalysis localJBBView is the same as mjoin's JBBSubset.
  CMPASSERT(mjoin->getJBBSubset() ==
            *(mjoin->getGroupAttr()->getGroupAnalysis()->getLocalJBBView()));

  // get a handle to the JBBSubset Analysis for the JBBSubset
  // represented by this MultiJoin (i.e. mjoin)
  JBBSubsetAnalysis * mjoinAnalysis = mjoin->
                                        getJBBSubset().
                                          getJBBSubsetAnalysis();

  // The list CANodeIdSets representing the join
  // sequence produced by this rule
  // Element 0: is inner most / top most element in left deep join sequence
  // Element N: is left most/outer most element in left deep join sequence
  const NAList<CANodeIdSet> * const leftDeepJoinSequence =
    mjoinAnalysis->getInitialPlanLeftDeepJoinSequence();

  UInt32 numEntriesInLeftDeepJoinSequence = (*leftDeepJoinSequence).entries();

  // setup the join directives for the left deep join sequence
  NAList<MJJoinDirective *> joinDirectives(CmpCommon::statementHeap(),
                                           numEntriesInLeftDeepJoinSequence-1);

  for(UInt32 i=0;
      i < (numEntriesInLeftDeepJoinSequence-1);
      i++)
  {
    MJJoinDirective * joinDirective =
      new (CmpCommon::statementHeap())
        MJJoinDirective(CmpCommon::statementHeap());

    CANodeIdSet joinRightChild = (*leftDeepJoinSequence)[i];
    JBBC * joinRightChildJBBC = NULL;
        
    if(joinRightChild.entries() == 1)
       joinRightChildJBBC = joinRightChild.getFirst().getNodeAnalysis()->getJBBC();

    // setup unconditional join directives
    joinDirective->setSkipJoinLeftShift();
    joinDirective->setJoinSource(Join::STAR_FILTER_JOIN);

    // Skip join commutativity if COMP_BOOL_3 is ON
    if (CmpCommon::getDefault(COMP_BOOL_3) == DF_ON)
    {
      joinDirective->setSkipJoinCommutativity();
    }

    // COMP_BOOL_85 is OFF my default therefore
    if ((CmpCommon::getDefault(COMP_BOOL_85) == DF_OFF) &&
        joinRightChildJBBC &&
        !(joinRightChildJBBC->getOriginalParentJoin() &&
          joinRightChildJBBC->getOriginalParentJoin()->isRoutineJoin()))
      joinDirective->setSkipNestedJoin();

    // COMP_BOOL_7 is OFF my default therefore
    // this is essentially unconditional
    if (CmpCommon::getDefault(COMP_BOOL_7) == DF_OFF)
      joinDirective->setJoinFromPTRule();

    if(joinRightChild.entries() > 1)
    {
      joinDirective->scheduleLSROnRightChild(MJStarJoinIRuleNumber);
    }

    // any special handling of the last join (i.e. last from the top)
    if(i == numEntriesInLeftDeepJoinSequence - 2)
    {
      CANodeIdSet joinLeftChild = (*leftDeepJoinSequence)[i+1];

      if(joinLeftChild.entries() > 1)
      {
        joinDirective->scheduleLSROnLeftChild(MJStarJoinIRuleNumber);
      }
    }

    joinDirectives.insert(joinDirective);

  }

  return mjoin->createLeftLinearJoinTree(leftDeepJoinSequence,
                                         &joinDirectives);
}

// Below is the old nextSubstitute. The code is not exercised any more
RelExpr * MJStarJoinIIRule::nextSubstitute_Old(RelExpr * before,
                                               Context * context,
                                               RuleSubstituteMemory * &memory)
{

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinIIRule_nextSubstitute_Begin" <<endl;
  }
#endif //_DEBUG

  MultiJoin * mjoin = (MultiJoin *) before;

  // Make sure GroupAnalysis localJBBView is the same as mjoin's JBBSubset.
  CMPASSERT(mjoin->getJBBSubset() ==
            *(mjoin->getGroupAttr()->getGroupAnalysis()->getLocalJBBView()));

  //Get a handle to ASM
  AppliedStatMan * appStatMan = QueryAnalysis::ASM();

  RelExpr * result = NULL;

  MJRulesWA * mjRulesWA = mjoin->
                            getJBBSubset().
                              getJBBSubsetAnalysis()->
                                getMJRulesWA();

  // get a handle to the JBBSubset Analysis for the JBBSubset
  // represented by this MultiJoin (i.e. mjoin)
  JBBSubsetAnalysis * mjoinAnalysis = mjoin->
                                        getJBBSubset().
                                          getJBBSubsetAnalysis();

  CANodeId factTable = mjRulesWA->factTable_;

  if(factTable == NULL_CA_ID)
    factTable = mjoinAnalysis->getLargestIndependentNode();

  CANodeIdSet factTableSet;
  factTableSet.insert(factTable);

  // Get list of JBBCs in this Multi Join, the list should be sorted by the
  // after local pred cardinality of the JBBC
  NAList<CANodeId> * sortedJBBCs = mjoinAnalysis->
                                     getNodesSortedByLocalPredsCard();

  // compute list of fringes

  //for tracking, which of the JBBCs in the multijoin are still available
  CANodeIdSet availableNodes = mjoin->getJBBSubset().getJBBCs();
  availableNodes.remove(factTable);

  // get tables directly connected to Fact Table
  CANodeIdSet connectedTables = factTable.getNodeAnalysis()->getJBBC()
                                                           ->getJoinedJBBCs();
  connectedTables += factTable.getNodeAnalysis()->getJBBC()
                                                ->getJBBCsRequiringInputFromMe();

  // remove connected tables from availableNodes
  // this is because we want to make sure only
  // one directly connected table per fringer
  availableNodes -= connectedTables;

  //List of fringes
  NAList<CANodeIdSet> listOfFringes(CmpCommon::statementHeap(),30);

  CANodeIdSet currentEdge;

  UInt32 lookAhead = (ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_18);

  for(UInt32 j=(*sortedJBBCs).entries(); j > 0; j--)
  {
    CANodeId jbbc = (*sortedJBBCs)[(j-1)];

    if (connectedTables.contains(jbbc))
    {
      NABoolean isLegalAddition = factTableSet.legalAddition(jbbc);

      if(!isLegalAddition)
      {
        availableNodes += jbbc;
        continue;
      }
      
      Join * jbbcParentJoin =
        jbbc.getNodeAnalysis()->getJBBC()->getOriginalParentJoin();

      currentEdge = jbbc;
      CANodeIdSet attachedNodes;

      //build the fringe
      if((!jbbcParentJoin) ||
         (jbbcParentJoin->isInnerNonSemiJoin()))
      {
        currentEdge = extendEdge(jbbc, availableNodes,lookAhead);

        // attachedNodes are nodes connected
        // to jbbc by join on their key
        attachedNodes =
          jbbc.getNodeAnalysis()->
            getJBBC()->
              getJBBCsConnectedViaKeyJoins();

        attachedNodes.intersectSet(availableNodes);
      }

      if (CmpCommon::getDefault(COMP_BOOL_49) == DF_OFF)
      {
        currentEdge += attachedNodes;
        availableNodes -= attachedNodes;
      }

      listOfFringes.insert(currentEdge);
#ifdef _DEBUG
      if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
           CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
      {
        CURRCONTEXT_OPTDEBUG->stream() << "Built a fringe starting from table "\
                        << jbbc.getText()<< endl;
        CURRCONTEXT_OPTDEBUG->stream() << "The fringe is " \
                        << currentEdge.getText()<<endl;
      }
#endif
    }
    else{
      continue;
    }
  }

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "Following tables are not part of any fringe, " \
                    << "put each of them as a fringe by itself: \n" \
                    << availableNodes.getText()<< endl;
  }
#endif

  // create fringes out of each one of the remaining tables
  // i.e. tables that have not been included as part of any
  // fringe
  for (CANodeId availableNode = availableNodes.init();
       availableNodes.next(availableNode);
       availableNodes.advance(availableNode))
  {
    listOfFringes.insert(availableNode);
  }

  // compute join order
  NAList<CANodeIdSet> orderedListOfFringes(STMTHEAP);
  orderedListOfFringes.insert(factTableSet);


  while (listOfFringes.entries())
  {
    // fringe that produces lowest cardinality when joined to the factTableSet
    CANodeIdSet smallestFringe;

    CostScalar lowestCardinality = 0;

    for (UInt32 i = 0;
         i < listOfFringes.entries();
         i ++)
    {
      CANodeIdSet currentFringe = listOfFringes[i];

      if(currentFringe.entries() == 1)
      {
        CANodeId fringeNode =
          currentFringe.getFirst();

        JBBC * fringeNodeJBBC =
          fringeNode.getNodeAnalysis()->getJBBC();

        CANodeIdSet jbbcsFringeNodeDependsOn =
          fringeNodeJBBC->getPredecessorJBBCs();

        if(!factTableSet.contains(jbbcsFringeNodeDependsOn))
          continue;
      }

      // get cardinality for joining factTableSet to currentFringe
      CostScalar joinCardinality =
        appStatMan->
          joinJBBChildren(factTableSet, currentFringe)->
            getResultCardinality();

      if((!smallestFringe.entries()) ||
         (joinCardinality < lowestCardinality))
      {
        lowestCardinality = joinCardinality;
        smallestFringe = currentFringe;
      }
    }

    listOfFringes.remove(smallestFringe);
    orderedListOfFringes.insert(smallestFringe);
    factTableSet.insert(smallestFringe);
  }

  // join order is not set, the left deep join order going
  // from the outer most to the inner most is:
  // Fact Table, orderedListOfFringes[0], orderedListOfFringest[1] ....., orderedListOfFringes[n]

  // construct the left deep join sequence going from the inner most -> outer most
  //

  // The list CANodeIdSets representing the join
  // sequence produced by this rule
  // Element 0: is inner most / top most element in left deep join sequence
  // Element N: is left most/outer most element in left deep join sequence
  NAList<CANodeIdSet> leftDeepJoinSequence (CmpCommon::statementHeap(),
                                        mjoin->getJBBSubset().getJBBCs().entries());

  for(UInt32 k = orderedListOfFringes.entries(); k > 0; k--)
  {
    leftDeepJoinSequence.insert(orderedListOfFringes[(k-1)]);
  }

  // Consolidate the fringes
  mjoinAnalysis->consolidateFringes(factTable, leftDeepJoinSequence);

  // end - construct left deep join sequence

  // setup the join directives for the left deep join sequence
  NAList<MJJoinDirective *> joinDirectives(CmpCommon::statementHeap(),orderedListOfFringes.entries()-1);

  UInt32 numEntriesInLeftDeepJoinSequence = leftDeepJoinSequence.entries();

  for(UInt32 i=0;
      i < (numEntriesInLeftDeepJoinSequence-1);
      i++)
  {
    MJJoinDirective * joinDirective = new MJJoinDirective(CmpCommon::statementHeap());
    CANodeIdSet joinRightChild = leftDeepJoinSequence[i];
    JBBC * joinRightChildJBBC = NULL;
        
    if(joinRightChild.entries() == 1)
       joinRightChildJBBC = joinRightChild.getFirst().getNodeAnalysis()->getJBBC();

    // setup unconditional join directives
    joinDirective->setSkipJoinLeftShift();
    joinDirective->setJoinSource(Join::STAR_FILTER_JOIN);

    // Skip join commutativity if COMP_BOOL_3 is ON
    if (CmpCommon::getDefault(COMP_BOOL_3) == DF_ON)
    {
      joinDirective->setSkipJoinCommutativity();
    }

    // COMP_BOOL_85 is OFF my default therefore
    if ((CmpCommon::getDefault(COMP_BOOL_85) == DF_OFF) &&
        joinRightChildJBBC &&
        !(joinRightChildJBBC->getOriginalParentJoin() &&
          joinRightChildJBBC->getOriginalParentJoin()->isRoutineJoin()))
      joinDirective->setSkipNestedJoin();

    // COMP_BOOL_7 is OFF my default therefore
    // this is essentially unconditional
    if (CmpCommon::getDefault(COMP_BOOL_7) == DF_OFF)
      joinDirective->setJoinFromPTRule();

    if(joinRightChild.entries() > 1)
    {
      joinDirective->scheduleLSROnRightChild(MJStarJoinIRuleNumber);
    }

    // any special handling of the last join (i.e. last from the top)
    if(i == numEntriesInLeftDeepJoinSequence - 2)
    {
      CANodeIdSet joinLeftChild = leftDeepJoinSequence[i+1];

      if(joinLeftChild.entries() > 1)
      {
        joinDirective->scheduleLSROnLeftChild(MJStarJoinIRuleNumber);
      }
    }

    joinDirectives.insert(joinDirective);

  }

  return mjoin->createLeftLinearJoinTree(&leftDeepJoinSequence,
                                         &joinDirectives);
}

// This class was used by old code, that is not exercised anymore
MJRulesWA::MJRulesWA(JBBSubsetAnalysis * analysis)
{
  CMPASSERT(analysis);
  analysis_ = analysis;

  centerTableComputed_ = FALSE;
  centerTable_ = NULL_CA_ID;
  centerTableRowsScanned_ = -1;
  centerTableDataScanned_ = -1;
  centerTableDataPerPartition_ = -1;
  centerTablePartitions_ = -1;
  centerTableConnectivity_ = 0,
  maxDimensionConnectivity_ = 0,


  factTable_ = NULL_CA_ID;
  largestTable_ = NULL_CA_ID;
  largestNode_ = NULL_CA_ID;
}
