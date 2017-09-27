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
* File:         MultiJoin.cpp
* Description:  MultiJoin Operator Methods
* Created:      03/03/03
* Language:     C++
*
*
******************************************************************************
*/

#include "MultiJoin.h"
#include "AppliedStatMan.h"
#include "RelJoin.h"

//
extern THREAD_P NAUnsigned              MJEnumRuleNumber;
extern THREAD_P NAUnsigned              MJStarJoinRuleNumber;
extern THREAD_P NAUnsigned              MJStarBDRuleNumber;
extern THREAD_P NAUnsigned              MJPrimeTableRuleNumber;
//extern NAUnsigned              MJPrimeTableRule2Number; Not needed
//

MultiJoin::MultiJoin(const JBBSubset & jbbSubset,
                     CollHeap *oHeap)
  : RelExpr(REL_MULTI_JOIN, NULL, NULL, oHeap)
  , jbbSubset_(jbbSubset)
  , childrenMap_(oHeap)
  , scheduledLSRs_(oHeap)
{
  // Need to initialize the childrenMap
  // This will set all children to NULL
  CANodeIdSet jbbcs = jbbSubset_.getJBBCs();
  Lng32 index = 0;

  for (CANodeId x= jbbcs.init();
       jbbcs.next(x);
       jbbcs.advance(x) )
  {
    JBBCExprGroupEntry* entry = new (oHeap)
      JBBCExprGroupEntry(x, (RelExpr*)NULL, oHeap);

    childrenMap_.insertAt(index, entry);
	index++;
  }

  lsrC_ = new (oHeap) LSRConfidence(oHeap);
#pragma warning (disable : 4018)  //warning elimination
  CMPASSERT (getArity() == jbbcs.entries());
#pragma warning (default : 4018)  //warning elimination
}

NABoolean MultiJoin::isSymmetricMultiJoin() const
{
  // now all are inners non semi
  return jbbSubset_.allJoinsInnerNonSemi();
}

void MultiJoin::pushdownCoveredExpr(const ValueIdSet & outputExpr,
                               const ValueIdSet & newExternalInputs,
                               ValueIdSet & predicatesOnParent,
			       const ValueIdSet * setOfValuesReqdByParent,
			       Lng32 childIndex
		              )
{
  CMPASSERT(0);
} // MultiJoin::pushdownCoveredExpr

void MultiJoin::getPotentialOutputValues(ValueIdSet & outputValues) const
{
  outputValues.clear();

  CANodeIdSet jbbcs = jbbSubset_.getJBBCs();

  Int32 arity = getArity();

  for (Lng32 i = 0; i < arity; i++)
  {
	  JBBC * jbbci =
        child(i)->getGroupAnalysis()->getNodeAnalysis()->getJBBC();

	  if(jbbci->parentIsLeftJoin())
        outputValues.insertList(jbbci->nullInstantiatedOutput());
      else
        // Default implementation is good enough for innerNonSemi multi join
        outputValues += child(i).getGroupAttr()->getCharacteristicOutputs();
  }
} // MultiJoin::getPotentialOutputValues()

const NAString MultiJoin::getText() const
{
  return NAString("multi_join");
} // MultiJoin::getText()

HashValue MultiJoin::topHash()
{
  selectionPred().clear();
  HashValue result = RelExpr::topHash();

  result ^= jbbSubset_.getJBBCs();
  result ^= CollIndex(jbbSubset_.getGB());

  return result;
}

NABoolean MultiJoin::duplicateMatch(const RelExpr & other) const
{
  // We need to compare the arity before RelExpr::duplicateMatch(...)
  // otherwise we may get out of array boundary exception when compaing
  // this multijoin to a RelExpr with different arity. This is different
  // than other RelExprs because the check for operator type does not
  // secure the same arity in this case.

  if (getArity() != other.getArity())
    return FALSE;

  ((RelExpr*)this)->selectionPred().clear();

  if (!RelExpr::duplicateMatch(other))
    return FALSE;

  MultiJoin &o = (MultiJoin &) other;

  if (jbbSubset_ != o.jbbSubset_)
    return FALSE;

  // At this point we will consider MJs with different LSRs to be different
  // This can change in the future, but would require updating the duplicate
  // MJ in the cascades memo to add the scheduledLSRs from the inserted one.
  if (scheduledLSRs_ != o.scheduledLSRs_)
    return FALSE;

  return TRUE;
}

RelExpr * MultiJoin::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  MultiJoin *result;

  if (derivedNode == NULL)
    result = new (outHeap) MultiJoin(jbbSubset_,outHeap);
  else
    result = (MultiJoin *) derivedNode;

  // I added scheduledLSRs_ too to be consistent with duplicateMatch
  // Read comment on duplicateMatch
  result->scheduledLSRs_ = scheduledLSRs_;

  return RelExpr::copyTopNode(result, outHeap);
}

Join* MultiJoin::splitSubset(const JBBSubset & leftSet,
                             const JBBSubset & rightSet,
                             NABoolean reUseMJ) const
{
  // At this point assert that none of the subsets has a group by member
  CMPASSERT ( (jbbSubset_.getGB() == NULL_CA_ID) &&
              (leftSet.getGB() == NULL_CA_ID) &&
              (rightSet.getGB() == NULL_CA_ID) );
#ifndef NDEBUG
  // assert that left + right == subSet_
  // and left intersect right = phi

  CANodeIdSet unionSet(leftSet.getJBBCs());
  CANodeIdSet intersectSet(leftSet.getJBBCs());

  unionSet += rightSet.getJBBCs();
  intersectSet.intersectSet(rightSet.getJBBCs());

  CMPASSERT ( (unionSet == jbbSubset_.getJBBCs()) &&
              (intersectSet.entries() == 0 ));
#endif

  // Note: Joins including left, semi, anti semi are only created when
  // a single jbbc connected via one of them is split as a single right
  // child. InnerNonSemi joins can be created for any split i.e. any
  // number of jbbcs on the left and the right of the join, but special
  // joins (i.e. left, semi and anti semi joins) are only created when
  // there is a single right child i.e. the rightSet contains only one
  // jbbc that is connected via a special join. This is enforced as follows
  //
  // * The leftSet should be legal: This means that for every jbbc in the
  //   leftSet any predecessor jbbcs should be present in the leftSet.
  // * The rightSet is either a single jbbc or if the rightSet has more
  //   than one jbbc then it should be legal, note that a jbbc connected
  //   via a special join is not a legal set by itself but we allow
  //   creation of special joins assuming the predecessors are present
  //   in the leftSet.
  //
  // An implicit assumption here is that 'this' MultiJoin is legal, which
  // is fair since apart from the top level multijoin, rest of the multijoins
  // are produced by splitting the top level multijoin. This method should
  // not produce illegal multijoins, since we check both leftSet and rightSet
  // for legality. Only time we don't check for legality is when the rightChild
  // is a single jbbc, and a single jbbc does not result in a multijoin.

  if(!leftSet.legal())
    return NULL;

  if((rightSet.getJBBCs().entries() > 1) && (!rightSet.legal()))
    return NULL;

  // everything here goes to statement heap
  CollHeap* outHeap = CmpCommon::statementHeap();

  RelExpr* child0 = generateSubsetExpr(leftSet, reUseMJ);
  RelExpr* child1 = generateSubsetExpr(rightSet, reUseMJ);

  // Flag to remember to pass on the derivedFromRoutineJoin flag if needed.
  NABoolean derivedFromRoutineJoin(FALSE);

  // now form a JoinExpr with these left and right children.
  Join * result = NULL;

  // if the rightSet is a single jbbc, then it could be connected via
  // a special join. In such a case we have to create the appropriate
  // join operator
  if(rightSet.getJBBCs().entries() == 1){

    JBBC * rightChild = rightSet.getJBBCs().getFirst().getNodeAnalysis()
                         ->getJBBC();

    Join * rightChildParentJoin = rightChild->getOriginalParentJoin();

    // If rightChildParentJoin is NULL, then the child is the left
    // child of the left most join and is considered to be connected
    // via a InnerNonSemi join.
    if(rightChildParentJoin)
    {
      if(rightChildParentJoin->derivedFromRoutineJoin())
        derivedFromRoutineJoin = TRUE;

      if(rightChildParentJoin->isSemiJoin())
        result = new (outHeap) Join(child0, child1, REL_SEMIJOIN, NULL);

      if(rightChildParentJoin->isAntiSemiJoin())
        result = new (outHeap) Join(child0, child1, REL_ANTI_SEMIJOIN, NULL);

      if(rightChildParentJoin->isLeftJoin())
      {

        // left joins can have filter preds, i.e. predicates that
        // are applied as filters after applying the join predicate.
        // We need to set them here.
        result = new (outHeap) Join(child0, child1, REL_LEFT_JOIN, NULL);
        result->setSelectionPredicates(rightChild->getLeftJoinFilterPreds());
      }
      
      if(rightChildParentJoin->isRoutineJoin())
      {
        derivedFromRoutineJoin = TRUE;
        result = new (outHeap) Join(child0, child1, REL_ROUTINE_JOIN, NULL);
        ValueIdSet routineJoinFilterPreds = rightChild->getRoutineJoinFilterPreds();
        ValueIdSet predsToAddToRoutineJoin;
          
        // add covered filter preds
        for (ValueId filterPred= routineJoinFilterPreds.init();
             routineJoinFilterPreds.next(filterPred);
             routineJoinFilterPreds.advance(filterPred) )
        {
          if(jbbSubset_.coversExpr(filterPred))
            predsToAddToRoutineJoin += filterPred;
        } 
 
        result->setSelectionPredicates(predsToAddToRoutineJoin);
      }

      if(result)
      {
        // set the join predicate for special joins, note predicates
        // for regular InnerNonSemi joins are set as selection predicates
        // in the join relexpr.
        result->setJoinPred(rightChild->getPredsWithPredecessors());

        result->nullInstantiatedOutput().insert(rightChild->
                                                  nullInstantiatedOutput());
      }
    }
  }

  // The join to be created is a regular InnerNonSemi join
  if (!result)
    result = new (outHeap) Join(child0, child1, REL_JOIN, NULL);

  // Make sure we carry the derivedFromRoutineJoin flag with us 
  if (derivedFromRoutineJoin)
    result->setDerivedFromRoutineJoin();

  // Share my groupAttr with result
  result->setGroupAttr(getGroupAttr());

  // get inner join predicates
  ValueIdSet selPreds = rightSet.joinPredsWithOther(leftSet);

  // get left join filter preds if any
  selPreds += result->getSelectionPredicates();

  result->setSelectionPredicates(selPreds);

  result->findEquiJoinPredicates();

  // May be I could save a little if i pushdown only to the child(ren)
  // that are not already JBBCs, i.e. multijoins
  result->pushdownCoveredExpr
    (result->getGroupAttr()->getCharacteristicOutputs(),
     result->getGroupAttr()->getCharacteristicInputs(),
     result->selectionPred());

  // We used CutOp as children, to avoid pushing predicates to JBBCs.
  // Now put the actual expression back in case the child is a JBBCs
  if(leftSet.getJBBCs().entries() ==  1)
    result->setChild(0, getJBBCRelExpr(leftSet.getJBBCs().getFirst()));

  // We used CutOp as children, to avoid pushing predicates to JBBCs.
  // Now put the actual expression back in case the child is a JBBCs
  if(rightSet.getJBBCs().entries() ==  1)
    result->setChild(1, getJBBCRelExpr(rightSet.getJBBCs().getFirst()));

  // Temp fixup. We need to take the selectionPred out of MultiJoin
  // for now to prevent that pushed expr from being there. selectionPred
  // is not being used now in MultiJoin xxx.
  if (leftSet.getJBBCs().entries() > 1)
    result->child(0)->selectionPred().clear();
  if (rightSet.getJBBCs().entries() > 1)
    result->child(1)->selectionPred().clear();

  return result;
}

RelExpr* MultiJoin::generateSubsetExpr(const JBBSubset & subset, NABoolean reUseMJ) const
{
  RelExpr* result;

  if (subset.getJBBCs().entries() == 1)
    result = getJBBCCutOpExpr(subset.getJBBCs().getFirst());
  else
    result = createSubsetMultiJoin(subset, reUseMJ);

  return result;
}

// --------------------------------------------------------------------
// Create a MultiJoin that is joining a subset of this MultiJoin children.
// Note: We assume JBBCs are already primed before calling this method.
// THis is a fine assumption at the analysis and post analysis stage.
// --------------------------------------------------------------------
MultiJoin* MultiJoin::createSubsetMultiJoin(const JBBSubset & subset, NABoolean reUseMJ) const
{
  MultiJoin * result = NULL;

  if(reUseMJ)
    result = subset.getSubsetMJ();

  if(result)
    return result;

  CMPASSERT (subset.getGB() == NULL_CA_ID); // no GBs for now
  CMPASSERT (subset.getJBBCs().entries() > 1);

  // everything here goes to statement heap
  CollHeap* outHeap = CmpCommon::statementHeap();

  result = new (outHeap) MultiJoin(subset, outHeap);
  result->setChildren(childrenMap_);

  result->setGroupAttr(new (outHeap) GroupAttributes());

  // Assign my Characteristic Inputs to the newly created subset multi-join.
  result->getGroupAttr()->addCharacteristicInputs
    (getGroupAttr()->getCharacteristicInputs());

  // The following call primes the result Characteristic Outputs.
  // It ensures that the inputs are minimal and outputs are maximal.
  result->primeGroupAttributes();
  // Now compute the GroupAnalysis fields
  result->primeGroupAnalysis();

  return result;
}

// --------------------------------------------------------------------
// use the input JBBCExprGroupMap to set this MultiJoin childrenMap_
// --------------------------------------------------------------------
void MultiJoin::setChildren(const JBBCExprGroupMap & map)
{
  // everything here goes to statement heap
  CollHeap* outHeap = CmpCommon::statementHeap();

  CANodeIdSet jbbcs = jbbSubset_.getJBBCs();

  CMPASSERT (map.getJBBCs().contains(jbbcs));

  Lng32 index = 0;

  for (CANodeId x= jbbcs.init();
       jbbcs.next(x);
       jbbcs.advance(x) )
  {
    JBBCExprGroupEntry* entry = new (outHeap)
      JBBCExprGroupEntry(x, map.getExprGroupIdOfJBBC(x), outHeap);

    childrenMap_.insertAt(index, entry);
	index++;
  }

  return;
}

// --------------------------------------------------------------------
// use origExprs from NodeAnalysis to set this MultiJoin childrenMap_
// --------------------------------------------------------------------
void MultiJoin::setChildrenFromOrigExprs(QueryAnalysis * qa)
{
  CollHeap* outHeap = qa->outHeap();

  CANodeIdSet jbbcs = jbbSubset_.getJBBCs();

  CMPASSERT (qa->getJBBCs().contains(jbbcs));

  Lng32 index = 0;

  for (CANodeId x= jbbcs.init();
       jbbcs.next(x);
       jbbcs.advance(x) )
  {
    JBBCExprGroupEntry* entry = new (outHeap)
      JBBCExprGroupEntry(x, qa->getNodeAnalysis(x)->getOriginalExpr(), outHeap);

    childrenMap_.insertAt(index, entry);
	index++;
  }

  return;
}

// -------------------------------------------------------------------
// This method returns the child as a RelExpr. If the child is
// a group it return a cut-op for that group.
// -------------------------------------------------------------------
RelExpr* MultiJoin::getJBBCRelExpr(CANodeId jbbc) const
{
  RelExpr* result = NULL;

  // everything here goes to statement heap
  CollHeap* outHeap = CmpCommon::statementHeap();

  ExprGroupId exprGroupId = childrenMap_.getExprGroupIdOfJBBC(jbbc);

  if (exprGroupId.getGroupId() == INVALID_GROUP_ID)
    result = exprGroupId.getPtr();
  else
  {
#pragma nowarn(1506)   // warning elimination
    result = new (outHeap) CutOp(exprGroupId.getGroupId(), outHeap);
#pragma warn(1506)  // warning elimination
    ((CutOp*)result)->setGroupIdAndAttr(exprGroupId.getGroupId());
    // may be we should re-use existing cut-op rather than creating
    // new one.
  }

  return result;
}


// -------------------------------------------------------------------
// This method returns the child as a cut-op.
// If the child is a group it return a cut-op for that group.
// If the child is a RelExpr it return a cut-op that shares its GA
// -------------------------------------------------------------------
CutOp* MultiJoin::getJBBCCutOpExpr(CANodeId jbbc) const
{
  // everything here goes to statement heap
  CollHeap* outHeap = CmpCommon::statementHeap();

  CutOp* result = new (outHeap) CutOp(0, outHeap); // we set index to 0 cuz it does not matter

  ExprGroupId exprGroupId = childrenMap_.getExprGroupIdOfJBBC(jbbc);

  if (exprGroupId.getGroupId() == INVALID_GROUP_ID)
    result->setExpr(exprGroupId.getPtr());
  else
    result->setGroupIdAndAttr(exprGroupId.getGroupId());

  return result;
}


// -----------------------------------------------------------------------
// MultJoin::recomputeOuterReferences()
// -----------------------------------------------------------------------
void MultiJoin::recomputeOuterReferences()
{
  // ---------------------------------------------------------------------
  // Delete all those input values that are no longer referenced on
  // this operator.
  // ---------------------------------------------------------------------
  if (NOT getGroupAttr()->getCharacteristicInputs().isEmpty())
  {
    ValueIdSet outerRefs = getGroupAttr()->getCharacteristicInputs();

    // Weed out those expressions not needed by my selectionPred and joinPred
    // xxx instead of taking this from getLocalJoinPreds, should I take it
    // from MultiJoin selectionPred??? refer to getLocalJoinPreds definition
    // and consider preds that referencing inputs!!!
    ValueIdSet exprSet = jbbSubset_.getLocalJoinPreds(); // from JbbSubsetAnalysis
    // Need to include LocalDependentPreds later when supported. Ok now for inner MultiJoins

    exprSet.weedOutUnreferenced(outerRefs);

    // Add back those expressiones needed by my children
    Int32 arity = getArity();

    // outputs produced by JBBCs in this MultiJoin
    ValueIdSet jbbcOutputs;
    for (Int32 i = 0; i < arity; i++)
    {
      outerRefs += child(i)->getGroupAttr()->getCharacteristicInputs();
      jbbcOutputs += child(i)->getGroupAttr()->getCharacteristicOutputs();
      // these inputs are provided by jbbcs in this MultiJoin
    }

    // account for TSJs i.e. values flowing from
    // one jbbc to another within this MultiJoin
    outerRefs -= jbbcOutputs;

    getGroupAttr()->setCharacteristicInputs(outerRefs);
  }
  return;
} // MultiJoin::recomputeOuterReferences()

// ------------------------------------------------------------------------
// Create a left linear subtree of joins from this MultiJoin
// ------------------------------------------------------------------------
Join* MultiJoin::leftLinearize(NABoolean fixJoinOrder, NABoolean createPriviledgedJoins) const
{
  // At this point assert that the subsets has no group by member.
  // If we want to allow GBs in this method in the future we should
  // remember to use computeJBBSubset() instead of the faster
  // jbbcsToJBBSubset() used currently in this method..
  CMPASSERT ( (jbbSubset_.getGB() == NULL_CA_ID));

  const CANodeIdSet & jbbcs = jbbSubset_.getJBBCs();
  // pick some child to make right child of join
  CANodeId jbbcRight(jbbcs.getFirst());

  CANodeIdSet right(jbbcRight);
  CANodeIdSet left(jbbcs);
  left -= jbbcRight;

  NABoolean nonExpander = (left.jbbcsToJBBSubset()->
    isGuaranteedNonExpandingJoin((*jbbcRight.getNodeAnalysis()->getJBBC())));

  Join* result = splitSubset(*(left.jbbcsToJBBSubset()),
                             *(right.jbbcsToJBBSubset()));

  if (fixJoinOrder)
  {
    // disallow left shift rule on the join
    result->contextInsensRules() += JoinLeftShiftRuleNumber;
    // disallow join commutativity on the join
    result->contextInsensRules() += JoinCommutativityRuleNumber;
  }

  if (createPriviledgedJoins)
  {
    result->setJoinFromPTRule();

    if ( CmpCommon::getDefault(COMP_BOOL_120) == DF_OFF)
    {
      result->updatePotential(-2);
    }
  }

  // If left subset is a multiJoin, then linearize it too.
  if (left.entries() > 1)
  {
    // left child must be multiJoin at this point. May be add assert.
    MultiJoin* leftChild = (MultiJoin*)(result->child(0).getPtr());
    result->child(0) = leftChild->leftLinearize(fixJoinOrder,createPriviledgedJoins);
  }

  return result;
} // MultiJoin::leftLinearize()

Join* MultiJoin::createLeftLinearJoinTree
                   (const NAList<CANodeIdSet> * const leftDeepJoinSequence,
                    NAList<MJJoinDirective *> * joinDirectives) const
{
  Join* result = NULL;

  Join* currentJoin=NULL;

  NABoolean reUseMultiJoins = FALSE;

  //Set of all JBBCs in this multi-join.
  //This set will be broken up to make the join tree
  //representing the substitue.
  //The loop below will construct the join tree,
  //starting from the top join.
  CANodeIdSet childSet = getJBBSubset().getJBBCs();

  // variables used in loop below
  MultiJoin * currentMJoin = (MultiJoin *) this;

  // in an iteration this is the parent join
  // e.g. when we are create JOIN3, this will
  // be JOIN2
  Join * parentJoin = NULL;

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "Following is left deep join sequence: " << endl;
    CURRCONTEXT_OPTDEBUG->stream() << endl;
  }
#endif

  UInt32 numJoinChildren = leftDeepJoinSequence->entries();

  CANodeId currentTable = NULL_CA_ID;

  for (UInt32 i = 0; i < (numJoinChildren-1); i++)
  {
    //create JBBSubset representing a comprising component of the
    //leftDeepJoinSequence.
    JBBSubset * joinRightChild = ((*leftDeepJoinSequence)[i]).computeJBBSubset();

    MJJoinDirective * joinDirective = (*joinDirectives)[i];

    //remove all tables that will become right side of join
    childSet.remove((*leftDeepJoinSequence)[i]);

#ifdef _DEBUG
    //print the right child of the current join
    if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
    {
      CURRCONTEXT_OPTDEBUG->stream() << ((*leftDeepJoinSequence)[i]).getText() << endl;
    }
#endif
    //Get JBBSubset for left side of join
    JBBSubset * joinLeftChild = childSet.computeJBBSubset();

    //create the join by doing a split of the multi-join
    currentJoin = currentMJoin->splitSubset(*joinLeftChild, *joinRightChild, reUseMultiJoins);

    joinDirective->setupJoin(currentJoin);

    if ( CmpCommon::getDefault(COMP_BOOL_120) == DF_OFF)
      currentJoin->updatePotential(-3);

    // if this is the first iteration
    // set the result to be the top join
    if (i == 0)
      result = currentJoin;

    //set the current multi-join to the left child of the
    //join just created
    //change getChild to child().getPointer
    currentMJoin = (MultiJoin*) currentJoin->getChild(0);

    //if there was a parent join, set the left child to
    //point to the new join we just created i.e. currentJoin.
    if (parentJoin)
      parentJoin->setChild(0,currentJoin);

    //set currentJoin to be the parent for the next iteration
    parentJoin = currentJoin;

  }

#ifdef _DEBUG
  //print the left most child
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << ((*leftDeepJoinSequence)[(numJoinChildren-1)]).getText() << endl;
    CURRCONTEXT_OPTDEBUG->stream() << endl;
  }
#endif

  // end - construct the join tree

  // synth the join
  result->synthLogProp();

  //if the right child of the top-most join is a multi-Join,
  //synthesize_it
  if(result->child(1))
    if(result->child(1)->getOperatorType()==REL_MULTI_JOIN)
      result->child(1)->synthLogProp();

  // synth the left child too
  result->child(0)->synthLogProp();

  return result;

} // MultiJoin::createLeftLinearJoinTree()


// ----------------------------------------------------------------
// method for testing purpose right now.
// Uses left linearize for expanding
// ----------------------------------------------------------------
RelExpr* MultiJoin::expandMultiJoinSubtree()
{
  // Use default implementation to invoke call on children
  RelExpr::expandMultiJoinSubtree();

  // Now do this operator
  Join* result = leftLinearize();
  return result;
}

// Methods for class MJJoinDirective
MJJoinDirective::MJJoinDirective(CollHeap *outHeap):
leftScheduledLSRs_(outHeap),
rightScheduledLSRs_(outHeap),
heap_(outHeap),
skipNestedJoin_(FALSE),
skipMergeJoin_(FALSE),
skipHashJoin_(FALSE),
skipJoinLeftShift_(FALSE),
skipJoinCommutativity_(FALSE),
joinSource_(Join::GENERAL),
joinFromPTRule_(FALSE)
{
}

void MJJoinDirective::setupJoin(Join * join)
{
  if(skipNestedJoin_)
    join->contextInsensRules() += JoinToTSJRuleNumber;

  if(skipMergeJoin_)
    join->contextInsensRules() += MergeJoinRuleNumber;

  if(skipHashJoin_)
    join->contextInsensRules() += HashJoinRuleNumber;

  if(skipJoinLeftShift_)
    join->contextInsensRules() += JoinLeftShiftRuleNumber;

  if(skipJoinCommutativity_)
    join->contextInsensRules() += JoinCommutativityRuleNumber;

  join->setSource(joinSource_);

  if(rightScheduledLSRs_.entries())
    ((MultiJoin*) (join->getChild(1)))->scheduledLSRs() += rightScheduledLSRs_;

  if(leftScheduledLSRs_.entries())
    ((MultiJoin*) (join->getChild(0)))->scheduledLSRs() += leftScheduledLSRs_;

  if(joinFromPTRule_)
    join->setJoinFromPTRule();

};

// xxx make this private
const ExprGroupId &
  JBBCExprGroupMap::getExprGroupIdOfJBBC(CANodeId jbbc) const
{

#pragma nowarn(1506)   // warning elimination
  Int32 entries = array_.entries();
#pragma warn(1506)  // warning elimination
  for (Int32 i = 0; i < entries; i++)
  {
    CMPASSERT(array_.used(i));
    if (array_[i]->getJBBCId() == jbbc)
    {
      return array_[i]->getExprGroupId();
    }
  }
  // Assert on invalid request
  CMPASSERT(FALSE);
  ExprGroupId* dummyResult = new(CmpCommon::statementHeap()) ExprGroupId;
  return *dummyResult;
}

// ---------------------------------------------------------------
// Compute the Group Analysis for this multi join based on its
// children. Verify its consistent with its jbbSubset_
// ---------------------------------------------------------------
void MultiJoin::primeGroupAnalysis()
{
  // Analysis must have passed now that we have a MultiJoin. Otherwise
  // we must have cleanup problem.
  CMPASSERT (QueryAnalysis::Instance()->isAnalysisON());

  // no GB in multi-joins for now
  CMPASSERT (jbbSubset_.getGB() == NULL_CA_ID);

  GroupAnalysis* groupAnalysis = getGroupAnalysis();

  // recursively call the children appending their subtreeTables
  // and parentJBBViews
  JBBSubset* localView =
    new (groupAnalysis->outHeap()) JBBSubset(groupAnalysis->outHeap());
  CANodeIdSet allSubtreeTables;
  Int32 arity = getArity();
  for (Lng32 i = 0; i < arity; i++)
  {
    GroupAnalysis* childAnalysis = child(i).getPtr()->getGroupAnalysis();
	// use children parentViews to build this join local view
    localView->addSubset(*(childAnalysis->getParentJBBView()));
    allSubtreeTables += childAnalysis->getAllSubtreeTables();
  }
  groupAnalysis->setLocalJBBView(localView);
  groupAnalysis->setSubtreeTables(allSubtreeTables);

  // The computed localJBBView should be the same as the MultiJoin subset.
  CMPASSERT (jbbSubset_ == *localView);

  return;
}

// this method is temp
CostScalar MultiJoin::getChildrenDataFlow() const
{
  CostScalar childrenDataFlow(0);

  UInt32 minRecordLength = (ActiveSchemaDB()->getDefaults())\
                                       .getAsLong(COMP_INT_50);
  Int32 arity = getArity();
  for (Int32 i = 0; i < arity; i++)
  {
    childrenDataFlow +=
      child(i)->getGroupAttr()->getResultCardinalityForEmptyInput() *
      MAXOF(child(i)->getGroupAttr()->getRecordLength(), minRecordLength);
  }

  return childrenDataFlow;
}

// ----------------------------------------------------------------
// SynthLogProp for multi join requires setting numJoinedTables
// ----------------------------------------------------------------
/*
void MultiJoin::synthLogProp()
{
  // Check to see whether this GA has already been associated
  // with a logExpr for synthesis.  If so, no need to resynthesize
  // for this equivalent log. expression.
  if (getGroupAttr()->existsLogExprForSynthesis()) return;

  CMPASSERT ( (jbbSubset_.getGB() == NULL_CA_ID));

  const CANodeIdSet & jbbcs = jbbSubset_.getJBBCs();
  CANodeId jbbcRight(jbbcs.getFirst());

  CANodeIdSet right(jbbcRight);
  CANodeIdSet left(jbbcs);
  left -= jbbcRight;

  Join* join = splitSubset(*(left.jbbcsToJBBSubset()),
                             *(right.jbbcsToJBBSubset()));

  join->synthLogProp();

  CMPASSERT ( getGroupAttr()->getNumJoinedTables() == getArity());
}
*/

Join * MultiJoin::getPreferredJoin()
{
  if(!getGroupAttr()->existsLogExprForSynthesis())
    synthLogProp();

  return (Join*) getGroupAttr()->getLogExprForSynthesis();
}

void MultiJoin::synthLogProp(NormWA * normWAPtr)
{
  // Check to see whether this GA has already been associated
  // with a logExpr for synthesis.  If so, no need to resynthesize
  // for this equivalent log. expression.
  if (getGroupAttr()->existsLogExprForSynthesis()) return;

  CMPASSERT ( (jbbSubset_.getGB() == NULL_CA_ID));

  const CANodeIdSet & jbbcs = jbbSubset_.getJBBCs();

  // Instead of always picking the first JBBC as the right child
  // pick the one with minimum JBBC connections. This will avoid
  // all unnecessary crossproducts

  CANodeId jbbcRight;

  jbbcRight = jbbcs.getJBBCwithMinConnectionsToThisJBBSubset();

  CANodeIdSet right(jbbcRight);
  CANodeIdSet left(jbbcs);
  left -= jbbcRight;

  Join* join = splitSubset(*(left.jbbcsToJBBSubset()),
                             *(right.jbbcsToJBBSubset()));

  join->synthLogProp(normWAPtr);

  getGroupAttr()->setLogExprForSynthesis(join);

  join->setJoinFromMJSynthLogProp();

  jbbSubset_.setSubsetMJ(this);
  
  CASortedList * synthLogPropPath =        
    new (CmpCommon::statementHeap()) 
    CASortedList(CmpCommon::statementHeap(), jbbcs.entries());
      
  synthLogPropPath->insert((*(left.jbbcsToJBBSubset()->getSynthLogPropPath())));
  synthLogPropPath->insert(right.getFirst());
  jbbSubset_.setSynthLogPropPath(synthLogPropPath);

  CMPASSERT ( getGroupAttr()->getNumJoinedTables() >= getArity());
}

void MultiJoin::synthLogPropWithMJReuse(NormWA * normWAPtr)
{
  // Check to see whether this GA has already been associated
  // with a logExpr for synthesis.  If so, no need to resynthesize
  // for this equivalent log. expression.
  if (getGroupAttr()->existsLogExprForSynthesis())
  {
    Join * joinExprForSynth = 
      (Join *) getGroupAttr()->getLogExprForSynthesis();
      
    if(joinExprForSynth->isJoinFromMJSynthLogProp())
      return;
  }

  NABoolean reUseMJ = TRUE;

  CMPASSERT ( (jbbSubset_.getGB() == NULL_CA_ID));

  const CANodeIdSet & jbbcs = jbbSubset_.getJBBCs();

  // Instead of always picking the first JBBC as the right child
  // pick the one with minimum JBBC connections. This will avoid
  // all unnecessary crossproducts

  CANodeId jbbcRight;

  jbbcRight = jbbcs.getJBBCwithMinConnectionsToThisJBBSubset();

  CANodeIdSet right(jbbcRight);
  CANodeIdSet left(jbbcs);
  left -= jbbcRight;

  Join* join = splitSubset(*(left.jbbcsToJBBSubset()),
                             *(right.jbbcsToJBBSubset()),
                           reUseMJ);

  //if the left is a MultiJoin, synthesize it using reUse
  //this has to be done before join->synthLogProp to avoid
  //calling MultiJoin::synthLogProp on the left MultiJoin
  //because that does not reUse
  if(left.entries() > 1)
  {
    RelExpr * leftRelExpr = join->child(0)->castToRelExpr();
    if(leftRelExpr &&
       leftRelExpr->getOperator() == REL_MULTI_JOIN)
      ((MultiJoin *) leftRelExpr)->synthLogPropWithMJReuse(normWAPtr);
  }

  join->synthLogProp(normWAPtr);

  join->setJoinFromMJSynthLogProp();

  getGroupAttr()->setLogExprForSynthesis(join);

  jbbSubset_.setSubsetMJ(this);

  CMPASSERT ( getGroupAttr()->getNumJoinedTables() >= getArity());
}

// -----------------------------------------------------------------------
// synthEstLogProp
// -----------------------------------------------------------------------

// Used for other RelExpr but not MultiJoin
void MultiJoin::synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp)
{
  CMPASSERT(inputEstLogProp->getNodeSet());

  Join * preferredJoin = jbbSubset_.getPreferredJoin();
  
  CMPASSERT(preferredJoin->isJoinFromMJSynthLogProp());
  
  EstLogPropSharedPtr myEstLogProp = 
    preferredJoin->getGroupAttr()->outputLogProp(inputEstLogProp);
  
  getGroupAttr()->addInputOutputLogProp (inputEstLogProp, myEstLogProp, NULL);
} // MultiJoin::synthEstLogProp


void MultiJoin::addLocalExpr(LIST(ExprNode *) &xlist,
			    LIST(NAString) &llist) const
{
  xlist.insert(jbbSubset_.getLocalJoinPreds().rebuildExprTree());
  llist.insert("local_multi_join_predicates");

  xlist.insert(jbbSubset_.getJoinPreds().rebuildExprTree());
  llist.insert("join_predicates_with_others");

  xlist.insert(jbbSubset_.getPredsWithPredecessors().rebuildExprTree());
  llist.insert("join_predicates_with_predecessors");

  xlist.insert(jbbSubset_.getPredsWithSuccessors().rebuildExprTree());
  llist.insert("join_predicates_with_successors");

  RelExpr::addLocalExpr(xlist,llist);
}

// -----------------------------------------------------------------------
// This method is used by the CQS re-write which re-arranged jbbcs
// in the join tree based on CQS tree.
// -----------------------------------------------------------------------
Join* MultiJoin::splitByTables(const CANodeIdSet & leftTableSet,
                               const CANodeIdSet & rightTableSet,
                               NABoolean reUseMJ) const
{
  // At this point assert that my subset has no group by member
  CMPASSERT ( jbbSubset_.getGB() == NULL_CA_ID );

#ifndef NDEBUG
  // assert that left + right == subSet_
  // and left intersect right = phi

  CANodeIdSet tables = jbbSubset_.getSubtreeTables();
  //CMPASSERT (tables == getGroupAnalysis()->getAllSubtreeTables());

  CANodeIdSet unionSet(leftTableSet);
  CANodeIdSet intersectSet(leftTableSet);

  unionSet += rightTableSet;
  intersectSet.intersectSet(rightTableSet);

  CMPASSERT ( (unionSet == tables) &&
              (intersectSet.entries() == 0 ));
#endif

  CANodeIdSet currentTables;
  JBBSubset leftJBBCs;
  JBBSubset rightJBBCs;

  const CANodeIdSet & jbbcs = jbbSubset_.getJBBCs();
  for (CANodeId x= jbbcs.init();  jbbcs.next(x); jbbcs.advance(x) )
  {
    currentTables = x.getNodeAnalysis()->getJBBC()->getSubtreeTables();
    if (leftTableSet.contains(currentTables))
    {
      // This child belong on the left
      leftJBBCs.addJBBC(x);
    }
	else if (rightTableSet.contains(currentTables))
    {
      // This child belong on the right
      rightJBBCs.addJBBC(x);
    }
    else
    {
      // not left, not right, this is illegal split request.
      return NULL;
    }
  }

  return splitSubset(leftJBBCs, rightJBBCs, reUseMJ);
} // MultiJoin::splitByTables()

// -----------------------------------------------------------------------
// analyzeInitialPlan
// -----------------------------------------------------------------------

void MultiJoin::analyzeInitialPlan()
{
  // get the subset analysis for this MultiJoin
  JBBSubsetAnalysis * subsetAnalysis = getJBBSubset().getJBBSubsetAnalysis();
  subsetAnalysis->analyzeInitialPlan(this);

  // analyzeInitialPlan for each child of the MultiJoin
  RelExpr::analyzeInitialPlan();
}

// ---------------------------------------------------------------------
// MultiJoinTester methods
// ---------------------------------------------------------------------
NABoolean MultiJoinTester::Test1(RelExpr* originalNonMultiJoinTree, RelExpr* treeConvertedToMultiJoin)
{
  RelExpr* treeCopy = treeConvertedToMultiJoin->copyRelExprTree(CmpCommon::statementHeap());
  treeCopy->synthLogProp();
  treeCopy->expandMultiJoinSubtree();
  treeCopy->synthLogProp();
  // Now assert both trees are the same
  CMPASSERT(originalNonMultiJoinTree->duplicateMatch(*treeCopy));
  return TRUE;
}
