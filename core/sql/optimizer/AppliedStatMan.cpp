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
* File:         AppliedStatMan.cpp
* Description:  Applied Statistics Manager (ASM)
*
* Created:      05/06/02
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "GroupAttr.h"
#include "RelGrby.h"
#include "AppliedStatMan.h"


//-----------------------------------------------------------------------------
//Methods on AppliedStatMan
//-----------------------------------------------------------------------------

// define a hash function for the cache

ULng32 AppliedStatMan::hashASM(const CANodeIdSet &key)
{
  return key.hash();
} // AppliedStatMan::hashASM

// This will be used when we are interested in only finding out
// if the statistics for the cache has already been created.
// Example in setupASMCacheForJBB.

NABoolean AppliedStatMan::lookup(const CANodeIdSet &key1) const
{
  //exist for the given key
  if (cacheASM_->contains (&key1))
    return TRUE;
  else
    return FALSE;
} // AppliedStatMan::lookup


// getCachedStatistics returns the pointer to estLogProp for the given
// nodeSet and the inputNodeSet. If the properties do not exist,
// it returns NULL.

EstLogPropSharedPtr AppliedStatMan::getCachedStatistics(
				const CANodeIdSet * combinedNodeSet)
{
  //exist for the given key
  if (cacheASM_->contains (combinedNodeSet))
  {

    EstLogPropSharedPtr cachedStat = cacheASM_->getFirstValue(combinedNodeSet);
    return cachedStat;
  }
  else
    return NULL;
} // AppliedStatMan::getCachedStatistics

// removeEntryIfThisObjectIsCached is used in the EstLogProp destructor. 
// This removes the reference to the key from the HashDictionary.

void AppliedStatMan::removeEntryIfThisObjectIsCached(EstLogProp * lp)
{
  if(!lp || (!lp->getNodeSet()))
    return;
  
  CANodeIdSet* nodeSet = lp->getNodeSet();
  if (cacheASM_->contains(nodeSet))
  {
    EstLogProp * cachedStat = cacheASM_->getFirstValue(nodeSet);
    if(lp == cachedStat)
      cacheASM_->remove(nodeSet);
  }
} // AppliedStatMan::removeEntryIfThisObjectIsCached


// Insert the pointer to outputEstLogProp in the cache. The key is
// the CANodeIdSet of the JBBCs for these EstLogProps. This consists
// of the CANodeIdSet of the left child, CANodeIdSet of the right
// child and CANodeIdSet of any outer children. This identifies
// the outputLogProp of the given JBBsubset for any given
// inputNodeSet

NABoolean AppliedStatMan::insertCachePredStatEntry(
			    const CANodeIdSet & jbbcNodeSet,
			    const EstLogPropSharedPtr& estLogProp)
{
  CANodeIdSet * tableSet = new (STMTHEAP) CANodeIdSet (jbbcNodeSet);
// FIXME!!! Must properly create cacheASM_
  CANodeIdSet * result = cacheASM_->
			insert(tableSet, estLogProp.get());

  if (result == NULL)
    return FALSE;  // insert failed.
  else
    return TRUE; // insert successful
} // AppliedStatMan::insertCachePredStatEntry


AppliedStatMan::AppliedStatMan(CollHeap *outHeap)

{
  // NOTE: Presently we are starting with the cache size of 107
  // We might like to change it later to depend on the number of
  // JBBCs. This would be especially useful for queries larger than
  // 32 tables, as this could avoid frequent resizing of the cache.

  //create the actual cache
   cacheASM_ = new (STMTHEAP)
     NAHashDictionary<CANodeIdSet, EstLogProp>
       (&(AppliedStatMan::hashASM),107,TRUE,outHeap);
}

// Get the scan expression for given jbbc
// if jbbc is not a scan or predIdSet is NULL
// then the original JBBC expression is returned
RelExpr * AppliedStatMan::getExprForCANodeId(
          CANodeId jbbc,
          const EstLogPropSharedPtr &inLP,
          const ValueIdSet * predIdSet)
{
  RelExpr * jbbcExpr = NULL;
  
  // should not happen but a check just in case
  CCMPASSERT(jbbc.getNodeAnalysis());
  
  //if specified by the user apply those predicates,
  // else apply predicates in the original expr
  NodeAnalysis * jbbcNode = jbbc.getNodeAnalysis();

  TableAnalysis * tableAnalysis = jbbcNode->getTableAnalysis();

  if (tableAnalysis && predIdSet)
  {
    TableDesc * tableDesc = tableAnalysis->getTableDesc();
    const CorrName& name = tableDesc->getNATable()->getTableName();

    Scan *scanExpr = new STMTHEAP Scan(name, tableDesc, REL_SCAN, STMTHEAP);
    scanExpr->setBaseCardinality(MIN_ONE (tableDesc->getNATable()->getEstRowCount())) ;

    GroupAttributes * gaExpr = new STMTHEAP GroupAttributes();

    scanExpr->setSelectionPredicates(*predIdSet);

    ValueIdSet requiredOutputs = jbbc.getNodeAnalysis()->\
  getOriginalExpr()->getGroupAttr()->getCharacteristicOutputs();

    gaExpr->setCharacteristicOutputs(requiredOutputs);

    ValueIdSet requiredInputs = jbbc.getNodeAnalysis()->\
  getOriginalExpr()->getGroupAttr()->getCharacteristicInputs();

    gaExpr->setCharacteristicInputs(requiredInputs);
    
    scanExpr->setGroupAttr(gaExpr);
    gaExpr->setLogExprForSynthesis(scanExpr);
    scanExpr->synthLogProp();
    jbbcExpr = scanExpr;
  }
  else
  {
    NodeAnalysis * nodeAnalysis = jbbc.getNodeAnalysis();

    RelExpr * relExpr = nodeAnalysis->getModifiedExpr();

    if (relExpr == NULL)
      relExpr = nodeAnalysis->getOriginalExpr();

    jbbcExpr = relExpr;
  }

  return jbbcExpr;
} // getExprForCANodeId

// AppliedStatMan::formJoinExprWithCANodeSet fakes a join expression,
// between the left and the right child. This method takes the left
// childId and the right childId, and forms a join expression.

Join * AppliedStatMan::formJoinExprWithCANodeSets(
					const CANodeIdSet & leftNodeSet,
					const CANodeIdSet & rightNodeSet,
					EstLogPropSharedPtr& inLP,
					const ValueIdSet * joinPreds,
					const NABoolean cacheable)
{
  EstLogPropSharedPtr leftEstLogProp = NULL;
  EstLogPropSharedPtr rightEstLogProp = NULL;

  CANodeIdSet * inputNodeSet = NULL;
  if (inLP->isCacheable())
  {
    inputNodeSet = inLP->getNodeSet();

    // if inLP are cacheable these should have a nodeSet attached
    // if it is not for some reason, assert in debug mode. In release
    // mode do not look for properties in ASM cache, instead get them
    // from group attr cache.
    if (inputNodeSet == NULL)
    {
      CCMPASSERT(inputNodeSet != NULL);
      inLP->setCacheableFlag(FALSE);
    }
  }

  CANodeIdSet commonNodeSet = leftNodeSet;
  commonNodeSet.intersectSet(rightNodeSet);

  // remove CANodeIds which are common to both left and the right children
  // from the child, whose estLogProps are not cached. If the estLogProps
  // of both children are not cached, then remove it from the child which
  // has a larger CANodeIdSet associated with it.

  CANodeIdSet tempLeftNodeSet = leftNodeSet;
  CANodeIdSet tempRightNodeSet = rightNodeSet;

  if (commonNodeSet.entries() > 0)
  {
    if (lookup(leftNodeSet))
      tempRightNodeSet.subtractSet(commonNodeSet);
    else
      if (lookup(rightNodeSet))
	tempLeftNodeSet.subtractSet(commonNodeSet);
      else
	if (leftNodeSet.entries() > rightNodeSet.entries())
	  tempLeftNodeSet.subtractSet(commonNodeSet);
	else
	  tempRightNodeSet.subtractSet(commonNodeSet);
  }

  // get the estLogProps for the left and the right child.
  // If these are not in the cache, then synthesize them incrementally
  // starting from the left most JBBC in the JBBSubset

  if (inputNodeSet)
  {
    // leftEstLogProp cached?

    CANodeIdSet combinedNodeSetWithInput = tempLeftNodeSet;
    combinedNodeSetWithInput.insert(*inputNodeSet);

    leftEstLogProp = getCachedStatistics(&combinedNodeSetWithInput);

    combinedNodeSetWithInput = tempRightNodeSet;
    combinedNodeSetWithInput.insert(*inputNodeSet);

    rightEstLogProp = getCachedStatistics(&combinedNodeSetWithInput);
  }

  if (leftEstLogProp == NULL)
      leftEstLogProp = synthesizeLogProp(&tempLeftNodeSet, inLP);

  // if the estimate logical properties have been computed for non-cacheable
  // inLP, then these would not contain nodeSet. But we do need the nodeSet
  // to compute potential output values. Hence we shall add this now

  if (!leftEstLogProp->getNodeSet())
  {
    CANodeIdSet * copyLeftNodeSet = new (STMTHEAP) CANodeIdSet (tempLeftNodeSet);
    leftEstLogProp->setNodeSet(copyLeftNodeSet);
  }

  if (rightEstLogProp == NULL)
      rightEstLogProp = synthesizeLogProp(&tempRightNodeSet, inLP);

  if (!rightEstLogProp->getNodeSet())
  {
    CANodeIdSet * copyRightNodeSet = new (STMTHEAP) CANodeIdSet (tempRightNodeSet);
    rightEstLogProp->setNodeSet(copyRightNodeSet);
  }

  // Now form the join expressions with these EstLogProp,
  // inLP and the joinPred will be same as those for which the
  // estLogProp are to be synthesized. Cacheable flag would depend
  // on whether left, right and the outer child are caheable, or
  // if the join is on all columns or not

  // Since the join expression consists of the left and the right
  // JBBSubsets, the JBBSubset for this Join expression would be
  // the superset of left and right JBBSubset

  JBBSubset * combinedSet = leftNodeSet.jbbcsToJBBSubset();
  combinedSet->addSubset(*(rightNodeSet.jbbcsToJBBSubset()));

  // Now form the join expressions with these EstLogProp,
  // inLP and the joinPred will be same as those for which the
  // estLogProp are to be synthesized. Cacheable flag would depend
  // on whether left, right and the outer child are ccaheable, or
  // if the join is on all columns or not

  return formJoinExprWithEstLogProps(leftEstLogProp, rightEstLogProp,
			    inLP, joinPreds, cacheable, combinedSet);



} // AppliedStatMan::formJoinExprWithCANodeSets

// This method forms the join expression with the estLogProps.
Join * AppliedStatMan::formJoinExprWithEstLogProps(
					const EstLogPropSharedPtr& leftEstLogProp,
					const EstLogPropSharedPtr& rightEstLogProp,
					const EstLogPropSharedPtr& inputEstLogProp,
					const ValueIdSet * setOfPredicates,
					const NABoolean cacheable,
					JBBSubset * combinedJBBSubset)
{
  // Form a join expression with these estLogProps.

  // form the left child. Since the estLogProps of the left and the
  // right children exist, these can be treated as Scan expressions

  Scan * leftChildExpr = new STMTHEAP Scan();
  GroupAttributes * galeft = new STMTHEAP GroupAttributes();

  // set GroupAttr of the leftChild
  galeft->inputLogPropList().insert(inputEstLogProp);
  galeft->outputLogPropList().insert(leftEstLogProp);
  CANodeIdSet * leftNodeSet = leftEstLogProp->getNodeSet();

  CANodeId nodeId;

  if (leftNodeSet)
  {
    if (leftNodeSet->entries() == 1)
    {
      nodeId = leftNodeSet->getFirst();
      if(nodeId.getNodeAnalysis()->getTableAnalysis())
	leftChildExpr->setTableAttributes(nodeId);
    }
    CostScalar minEstCard = leftNodeSet->getMinChildEstRowCount();

    galeft->setMinChildEstRowCount(minEstCard);
  }

  leftChildExpr->setGroupAttr(galeft);
  galeft->setLogExprForSynthesis(leftChildExpr);

  // form the right child and set its groupAttr
  Scan * rightChildExpr = new STMTHEAP Scan();
  GroupAttributes * garight = new STMTHEAP GroupAttributes();
  garight->inputLogPropList().insert(inputEstLogProp);
  garight->outputLogPropList().insert(rightEstLogProp);
  CANodeIdSet * rightNodeSet = rightEstLogProp->getNodeSet();

  // xxx

  JBBC * singleRightChild = NULL;
  Join * singleRightChildParentJoin = NULL;
  ValueIdSet leftOuterJoinFilterPreds;


  if (rightNodeSet)
  {
    if (rightNodeSet->entries() == 1)
    {
      nodeId = rightNodeSet->getFirst();
      if(nodeId.getNodeAnalysis()->getTableAnalysis())
	rightChildExpr->setTableAttributes(nodeId);
	  if(nodeId.getNodeAnalysis()->getJBBC())
	  {
		  singleRightChild = nodeId.getNodeAnalysis()->getJBBC();
		  if(singleRightChild)
		    singleRightChildParentJoin = singleRightChild->getOriginalParentJoin();
	  }
    }
    CostScalar minEstCard = rightNodeSet->getMinChildEstRowCount();

    garight->setMinChildEstRowCount(minEstCard);
  }

  rightChildExpr->setGroupAttr(garight);
  garight->setLogExprForSynthesis(rightChildExpr);

  Join * joinExpr = NULL;
  if(singleRightChild &&
	 singleRightChildParentJoin)
  {
      if(singleRightChildParentJoin->isSemiJoin())
        joinExpr = new STMTHEAP Join(leftChildExpr,
                                     rightChildExpr,
                                     REL_SEMIJOIN,
                                     NULL);

      if(singleRightChildParentJoin->isAntiSemiJoin())
        joinExpr = new STMTHEAP Join(leftChildExpr,
                                     rightChildExpr,
                                     REL_ANTI_SEMIJOIN,
                                     NULL);

      if(singleRightChildParentJoin->isLeftJoin())
      {
        joinExpr = new STMTHEAP Join(leftChildExpr,
			                          rightChildExpr,
									  REL_LEFT_JOIN,
									  NULL);
        leftOuterJoinFilterPreds += singleRightChild->getLeftJoinFilterPreds();
      }

      if(joinExpr)
      {
        joinExpr->setJoinPred(singleRightChild->getPredsWithPredecessors());

        joinExpr->nullInstantiatedOutput().insert(singleRightChild->
                                                    nullInstantiatedOutput());
      }
  }

  if(!joinExpr)
  {
  // now form a JoinExpr with these left and right children.
  joinExpr = new STMTHEAP Join(leftChildExpr,  // left child
				      rightChildExpr, // right child
				      REL_JOIN,	      // join type
				      NULL);	      // join predicates
  }

  ValueIdSet selPredsAndLOJFilter = leftOuterJoinFilterPreds;
  selPredsAndLOJFilter += (*setOfPredicates);
  joinExpr->setSelectionPredicates(selPredsAndLOJFilter);

  // set groupAttr of this Join expression
  GroupAttributes * gaJoin = new STMTHEAP GroupAttributes();

  // set required outputs of Join as sum of characteristic
  // outputs of the left and the right children
  ValueIdSet requiredOutputs;

  if (leftNodeSet)
    requiredOutputs.addSet(getPotentialOutputs(*(leftNodeSet)));

  if (rightNodeSet)
    requiredOutputs.addSet(getPotentialOutputs(*(rightNodeSet)));

  gaJoin->setCharacteristicOutputs(requiredOutputs);

  // set JBBSubset for this group, if all estLogProps are cacheable.
  // Else JBBSubset is NULL

  if (cacheable)
    gaJoin->getGroupAnalysis()->setLocalJBBView(combinedJBBSubset);

  gaJoin->setMinChildEstRowCount(MINOF(garight->getMinChildEstRowCount(), galeft->getMinChildEstRowCount() ) );

  joinExpr->setGroupAttr(gaJoin);

  // if there are some probes coming into the join
  // then join type = tsj.
  if ((inputEstLogProp->getResultCardinality() > 1) ||
      (inputEstLogProp->getColStats().entries() > 1))
  {
    if (cacheable)
    {
      CANodeIdSet inputNodeSet =  *(inputEstLogProp->getNodeSet());
      gaJoin->setCharacteristicInputs(getPotentialOutputs(inputNodeSet));
    }
  }

  joinExpr->setGroupAttr(gaJoin);
  gaJoin->setLogExprForSynthesis(joinExpr);
  return joinExpr;

} // AppliedStatMan::formJoinExprWithEstLogProps

// This method forms the join expression for join on JBBC specified by jbbcId
// inputEstLogProp should not be cacheable
Join * AppliedStatMan::formJoinExprForJoinOnJBBC(
          CANodeIdSet jbbSubset,
          CANodeId    jbbcId,
          const ValueIdSet * jbbcLocalPreds,
          const ValueIdSet * joinPreds,
          const EstLogPropSharedPtr& inputEstLogProp,
          const NABoolean cacheable)
{

  NABoolean origInputIsCacheable = inputEstLogProp->isCacheable();
  if(origInputIsCacheable)
  {
    inputEstLogProp->setCacheableFlag(FALSE);
    CCMPASSERT("Expecting Non Cacheable Input");
  }
  
  RelExpr * jbbcExpr = getExprForCANodeId(jbbcId, inputEstLogProp, jbbcLocalPreds);
  jbbcExpr->getGroupAttr()->outputLogProp(inputEstLogProp);
  RelExpr * jbbSubsetExpr = jbbSubset.jbbcsToJBBSubset()->getPreferredJoin();
  
  if(!jbbSubsetExpr)
    if(jbbSubset.entries()==1)
      if(!inputEstLogProp->isCacheable())
      {
        inputEstLogProp->setCacheableFlag(TRUE);
        jbbSubsetExpr = getExprForCANodeId(jbbSubset.getFirst(), inputEstLogProp);
        inputEstLogProp->setCacheableFlag(FALSE);
      }
      else
        jbbSubsetExpr = getExprForCANodeId(jbbSubset.getFirst(), inputEstLogProp);
    else
    {
      CCMPASSERT("No Subset expression, need at least one entry in set");
    }


  RelExpr * leftChildExpr = jbbSubsetExpr;
  RelExpr * rightChildExpr = jbbcExpr;
  
  GroupAttributes * galeft = jbbSubsetExpr->getGroupAttr();
  GroupAttributes * garight = jbbcExpr->getGroupAttr();
  
  // xxx

  JBBC * jbbc = jbbcId.getNodeAnalysis()->getJBBC();
  Join * jbbcParentJoin = jbbc->getOriginalParentJoin();
  ValueIdSet leftOuterJoinFilterPreds;


  Join * joinExpr = NULL;
  
  if(jbbcParentJoin)
  {
      if(jbbcParentJoin->isSemiJoin())
        joinExpr = new STMTHEAP Join(leftChildExpr, rightChildExpr, REL_SEMIJOIN, NULL);

      if(jbbcParentJoin->isAntiSemiJoin())
        joinExpr = new STMTHEAP Join(leftChildExpr, rightChildExpr, REL_ANTI_SEMIJOIN, NULL);

      if(jbbcParentJoin->isLeftJoin())
      {
        joinExpr = new STMTHEAP Join(leftChildExpr, rightChildExpr, REL_LEFT_JOIN, NULL);
        leftOuterJoinFilterPreds += jbbc->getLeftJoinFilterPreds();
      }

      if(joinExpr)
      {
        joinExpr->setJoinPred(jbbc->getPredsWithPredecessors());

        joinExpr->nullInstantiatedOutput().insert(jbbc->nullInstantiatedOutput());
      }
  }

  if(!joinExpr)
  {
    // now form a JoinExpr with these left and right children.
    joinExpr = new STMTHEAP Join(leftChildExpr, rightChildExpr, REL_JOIN, NULL);
  }

  ValueIdSet selPredsAndLOJFilter = leftOuterJoinFilterPreds;
  selPredsAndLOJFilter += (*joinPreds);
  joinExpr->setSelectionPredicates(selPredsAndLOJFilter);

  // set groupAttr of this Join expression
  GroupAttributes * gaJoin = new STMTHEAP GroupAttributes();

  // set required outputs of Join as sum of characteristic
  // outputs of the left and the right children
  ValueIdSet requiredOutputs;

  requiredOutputs.addSet(getPotentialOutputs(jbbSubset));

  requiredOutputs.addSet(getPotentialOutputs(jbbcId));

  gaJoin->setCharacteristicOutputs(requiredOutputs);

  // set JBBSubset for this group, if all estLogProps are cacheable.
  // Else JBBSubset is NULL

  CANodeIdSet combinedSet = jbbSubset;
  combinedSet += jbbcId;
  
  if (cacheable)
    gaJoin->getGroupAnalysis()->setLocalJBBView(combinedSet.jbbcsToJBBSubset());

  gaJoin->setMinChildEstRowCount(MINOF(garight->getMinChildEstRowCount(), galeft->getMinChildEstRowCount() ) );

  // if there are some probes coming into the join
  // then join type = tsj.
  if ((inputEstLogProp->getResultCardinality() > 1) ||
      (inputEstLogProp->getColStats().entries() > 1))
  {
    if (cacheable)
    {
      CANodeIdSet inputNodeSet =  *(inputEstLogProp->getNodeSet());
      gaJoin->setCharacteristicInputs(getPotentialOutputs(inputNodeSet));
    }
  }

  joinExpr->setGroupAttr(gaJoin);
  gaJoin->setLogExprForSynthesis(joinExpr);
  joinExpr->synthLogProp();
  inputEstLogProp->setCacheableFlag(origInputIsCacheable);
  return joinExpr;
} // AppliedStatMan::formJoinExprForJoinOnJBBC

// synthesizeLogProp method is used to synthesize estLogProps
// for the JBBSubset if these do not already exist in the ASM
// cache. All local predicates are applied
// on the JBBCs and the join is done on all columns

EstLogPropSharedPtr AppliedStatMan::synthesizeLogProp(
				const CANodeIdSet * nodeSet,
				EstLogPropSharedPtr &inLP)
{
  EstLogPropSharedPtr outputEstLogProp;
  CANodeIdSet combinedNodeSetWithInput = *nodeSet;

  if (inLP->isCacheable())
  {
    CANodeIdSet * inNodeSet = inLP->getNodeSet();

    // if inLP are cacheable these should have a nodeSet attached
    // if not, assert in debug mode. In release mode, set the properties
    // as not cacheable. These will then be looked into group attr cache
    if (inNodeSet == NULL)
    {
      CCMPASSERT(inNodeSet != NULL);
      inLP->setCacheableFlag(FALSE);
    }
    else
    {
      // check ASM cache for the estLogProps of nodeSet for the given
      // inLP

      combinedNodeSetWithInput.insert(*inNodeSet);
      if ((outputEstLogProp =\
        getCachedStatistics(&combinedNodeSetWithInput)) != NULL)
      return outputEstLogProp;
    }
  }

	if(nodeSet->entries() == 1)
    return getStatsForCANodeId(nodeSet->getFirst(), inLP);

  JBBSubset * jbbSubset = nodeSet->jbbcsToJBBSubset();

  Join * preferredJoin = jbbSubset->getPreferredJoin();

  //CMPASSERT(preferredJoin->isJoinFromMJSynthLogProp());

  outputEstLogProp = preferredJoin->getGroupAttr()->outputLogProp(inLP);

	return outputEstLogProp;
} // AppliedStatMan::synthesizeLogProp

EstLogPropSharedPtr AppliedStatMan::joinEstLogProps (
              const EstLogPropSharedPtr& leftEstLogProp,
              const EstLogPropSharedPtr& rightEstLogProp,
              const EstLogPropSharedPtr& inLP)
{
  EstLogPropSharedPtr outputEstLogProp;

  NABoolean cacheable = FALSE;

  CANodeIdSet * inputNodeSet = inLP->getNodeSet();

  // These nodesets could be NULL, if the estLogProps to which they
  // belong are not cacheable

  CANodeIdSet * leftNodeSet = leftEstLogProp->getNodeSet();
  CANodeIdSet * rightNodeSet = rightEstLogProp->getNodeSet();

  if ((leftEstLogProp->isCacheable()) &&
     (rightEstLogProp->isCacheable()) &&
     (inLP->isCacheable()) )
  {
    CCMPASSERT(leftNodeSet != NULL);
    CCMPASSERT(rightNodeSet != NULL);
    CCMPASSERT(inputNodeSet != NULL);
    if (leftNodeSet && rightNodeSet && inputNodeSet)
    {
      cacheable = TRUE;
    }
  }

  if (cacheable)
  {
    // check the ASM cache to see if outputEstLogProp for these
    // NodeSets appear for the given inputEstLogProp

    CANodeIdSet combineNodeSet = *leftNodeSet;
    combineNodeSet.insert(*rightNodeSet);

    CANodeIdSet combinedWithInputNodeSet = combineNodeSet;
    combinedWithInputNodeSet.insert(*inputNodeSet);

    outputEstLogProp = getCachedStatistics(&combinedWithInputNodeSet);
    if (outputEstLogProp != NULL)
      return outputEstLogProp;
  }

  JBBSubset * newJBBSubset = NULL;

  ValueIdSet setOfPredicates;

  if  (leftNodeSet && rightNodeSet)
  {
    // join predicates can be obtained from EstLogProp, only
    // if these corresponded to complete set of predicates -
    // all local or complete join. Also, we need a
    // combinedJBBSubset to set in the fake join expression
    // that we will be creating.

    newJBBSubset = leftNodeSet->computeJBBSubset();
    JBBSubset rightJBBSubset = *(rightNodeSet->computeJBBSubset());
    setOfPredicates = newJBBSubset->joinPredsWithOther(rightJBBSubset);

    // Since the properties from this group are cacheable, hence the
    // group attributes for the new join expression should contain
    // the combined JBBsubset of the left and the right children

    newJBBSubset->addSubset(rightJBBSubset);
  }

  // inputEstLogProp would be either empty input estLogProp or from the
  // outer child. If cacheable is TRUE, then newJBBsubset should
  // contain the combined left and the right JBB subset. But if
  // cacheable is FALSE, newJBBsubset should be NULL

  Join * joinExpr = formJoinExprWithEstLogProps(
				      leftEstLogProp,
				      rightEstLogProp,
				      inLP,
				      &setOfPredicates,
				      cacheable,
				      newJBBSubset);

  // Now do the actual synthesis and cache statistics in the cache

  outputEstLogProp = joinExpr->getGroupAttr()->outputLogProp(inLP);
  return outputEstLogProp;
}

// AppliedStatMan::getPotentialOutputs. This method is called from
// formJoinExpr methods. It sets the characteristics output of the
// join expr, which is equal to the union of the characteristic
// outputs of the left and the right JBBsubsets. jbbcsNodeSet is
// the combined nodeSet of the left and the right CANodeIdSets

ValueIdSet AppliedStatMan::getPotentialOutputs(
			  const CANodeIdSet & jbbcsNodeSet)
{
  ValueIdSet potentialOutputs;

  for (CANodeId jbbc = jbbcsNodeSet.init();
		    jbbcsNodeSet.next(jbbc);
		    jbbcsNodeSet.advance(jbbc))
  {
    if (NodeAnalysis * jbbcNodeAnalysis = jbbc.getNodeAnalysis())
    {
      ValueIdSet outputs;
	  const Join * jbbcParentJoin = jbbcNodeAnalysis->getJBBC()->
                                      getOriginalParentJoin();
      if((!jbbcParentJoin) ||
		 (jbbcParentJoin && jbbcParentJoin->isInnerNonSemiJoin()))
        outputs = jbbcNodeAnalysis->getOriginalExpr()->\
          getGroupAttr()->getCharacteristicOutputs();
	  else if (jbbcParentJoin->isLeftJoin())
        outputs = jbbcParentJoin->nullInstantiatedOutput();
      potentialOutputs.insert(outputs);
    }
  }

  return potentialOutputs;
} // AppliedStatMan::getPotentialOutputs


// AppliedStatMan::setupASMCacheForJBB method will be called from
// Query::Analyze after connectivity analysis has been done and
// empty logical properties have been set.
void AppliedStatMan::setupASMCacheForJBB(JBB & jbb)
{
  EstLogPropSharedPtr myEstLogProp;

  // get all JBBCs of JBB
  const CANodeIdSet jbbcNodeIdSet = jbb.getMainJBBSubset().getJBBCs();
  CANodeId jbbcId;

  // for all jbbcs
  for (jbbcId = jbbcNodeIdSet.init();
	  jbbcNodeIdSet.next(jbbcId);
	  jbbcNodeIdSet.advance(jbbcId))
  {
    if (NodeAnalysis * jbbcNode = jbbcId.getNodeAnalysis())
    {
      // Evaluate local predicates only if it is a table.

      RelExpr * jbbcExpr = jbbcNode->getOriginalExpr();

      if ((jbbcNode->getTableAnalysis() != NULL) &&
	        (jbbcExpr->getOperatorType() == REL_SCAN))
      {
        // get the original expression of the jbbc
        Scan * scanExpr = (Scan *) jbbcExpr;

        ValueIdSet localPreds = scanExpr->getSelectionPredicates();

        // if local predicates have already been computed, then skip
        if ((localPreds.entries() > 0) || !(lookup(jbbcId)))
        {
          // check to see this GA has already been associated with
          // a logExpr for synthesis.  If not, then synthesize
	        // log. expression, and then apply local predicates to it

          if (NOT scanExpr->getGroupAttr()->existsLogExprForSynthesis())
	          scanExpr->synthLogProp();

	        myEstLogProp = getStatsForCANodeId(jbbcId);
	      }
      }
    }
  }

  // Now do a second traversal of the JBB looking for join reducers
  for (jbbcId = jbbcNodeIdSet.init();
		jbbcNodeIdSet.next(jbbcId);
		jbbcNodeIdSet.advance(jbbcId))
  {
    // now look for all two way joins for this child
    if (jbbcId.getNodeAnalysis())
    {

      // get all JBBCs connected to this JBBC, and do a two-way
      // join with all of them

      CANodeIdSet connectedNodes = jbbcId.getNodeAnalysis()->\
				  getJBBC()->getJoinedJBBCs();

      for (CANodeId connectedTable = connectedNodes.init();
			      connectedNodes.next(connectedTable);
			      connectedNodes.advance(connectedTable))
      {
	      if (connectedTable.getNodeAnalysis())
	      {

	        // ASM does not concern itself with the order of the tables,
	        // hence it is possible that the join has already been computed

	        CANodeIdSet tableSet = jbbcId;
	        tableSet.insert(connectedTable);

	        if ((myEstLogProp = getCachedStatistics(&tableSet)) == NULL)
	        {
	          CANodeIdSet setForjbbcId(jbbcId);
	          CANodeIdSet setForConnectedTable(connectedTable);
	          myEstLogProp = joinJBBChildren(setForjbbcId, setForConnectedTable);
	        }
	      }
      }
    }
  }
} // AppliedStatMan::setupASMCacheForJBB


// Following three methods can be used to get cached statistics from
// ASM cache for JBBsubset / CANodeIdSet / CANodeId. In all cases
// if the properties do not exist in the cache, these are synthesized
// here. As the ASM does not have access to the expression
// for this JBBsubset, the properties are synthesized incrementally
// which can be very inefficient, hence this method should not be used
// as a substitute for joinJBBSubsets / joinCANodeIdSets

EstLogPropSharedPtr AppliedStatMan::getStatsForJBBSubset(
					const JBBSubset & jbbSubset)
{
  CANodeIdSet jbbNodeSet = jbbSubset.getJBBCs();

  // We don't want any group-bys at this stage. 
  CMPASSERT ( (jbbSubset.getGB() == NULL_CA_ID));

  return getStatsForCANodeIdSet(jbbNodeSet);
};

// this method assume jbbNodeSet contains nodes from the same JBB
EstLogPropSharedPtr AppliedStatMan::getStatsForCANodeIdSet(
					const CANodeIdSet & jbbNodeSet)
{

  EstLogPropSharedPtr outputEstLogProp;
  CANodeIdSet combinedNodeSet = jbbNodeSet;
  combinedNodeSet += *(jbbNodeSet.getJBBInput()->getNodeSet());
  EstLogPropSharedPtr jBBInput = jbbNodeSet.getJBBInput();
  if ((outputEstLogProp = getCachedStatistics(&combinedNodeSet)) == NULL)
    outputEstLogProp = synthesizeLogProp(&jbbNodeSet, jBBInput);

  return outputEstLogProp;
}

// Following method is used to join two CANodeIdSets. This is less
// expensive than joinJBBSubsets as CANodeIdSets are lighter
// structures, and it assumes complete join is to be done
// But the user should be careful while sending in leftJBBSubsets
// and rightJBBSubsets. Both these CANodeIdSets should correspond
// to the JBBCs from the same JBB.

EstLogPropSharedPtr AppliedStatMan::joinJBBChildren(
					const CANodeIdSet & leftChildren,
					const CANodeIdSet & rightChildren,
					EstLogPropSharedPtr & inLP)
{

  EstLogPropSharedPtr inputLP = inLP;

  EstLogPropSharedPtr outputEstLogProp;

  if(inputLP == (*GLOBAL_EMPTY_INPUT_LOGPROP))
    inputLP = leftChildren.getJBBInput();

  // Because there exist a nodeSet for the left, right and the outer
  // child, hence these properties are cacheable. Check to see if the
  // outputEstLogProp of the join for the given inLP exist in the cache

  CANodeIdSet combinedNodeSet = leftChildren;
  combinedNodeSet.insert(rightChildren);

  CANodeIdSet * inNodeSet = NULL;

  if (inputLP->isCacheable())
  {
    inNodeSet = inputLP->getNodeSet();

    CANodeIdSet combinedWithInputNodeSet = combinedNodeSet;
    combinedWithInputNodeSet.insert(*inNodeSet);

    outputEstLogProp = getCachedStatistics(&combinedWithInputNodeSet);
  }

  if(outputEstLogProp == NULL)
    outputEstLogProp = synthesizeLogProp(&combinedNodeSet, inputLP);
  
  return outputEstLogProp;
} // AppliedStatMan::joinJBBChildren

  // do a fast computation of the join reduction based only on the
  // jbbcs that are involved in the join between the two sets

CostScalar AppliedStatMan::computeJoinReduction(
          const CANodeIdSet & leftChildren,
          const CANodeIdSet & rightChildren)
{
  CostScalar result = 0;

  // get stats for left
  EstLogPropSharedPtr leftCard =
    getStatsForCANodeIdSet(leftChildren);

  // get stats for right
  EstLogPropSharedPtr rightCard =
    getStatsForCANodeIdSet(rightChildren);

  CANodeIdSet jbbcsJoinedToRight;
  CANodeIdSet allPredecessors;
  CANodeIdSet allSuccessors;

  for( CANodeId rChild = rightChildren.init();
       rightChildren.next(rChild);
       rightChildren.advance(rChild))
  {
    JBBC * rChildJBBC = rChild.getNodeAnalysis()->getJBBC();
    jbbcsJoinedToRight += rChildJBBC->getJoinedJBBCs();
    jbbcsJoinedToRight += rChildJBBC->getPredecessorJBBCs();
    allPredecessors    += rChildJBBC->getPredecessorJBBCs();
    jbbcsJoinedToRight += rChildJBBC->getSuccessorJBBCs();
    allSuccessors      += rChildJBBC->getSuccessorJBBCs();
  }

  CANodeIdSet dependencyCausingNodesFromLeft = leftChildren;
  dependencyCausingNodesFromLeft.intersectSet(allPredecessors + allSuccessors);

  CANodeIdSet leftNodesJoinedToRight = leftChildren;
  leftNodesJoinedToRight.intersectSet(jbbcsJoinedToRight);

  if(!leftNodesJoinedToRight.entries())
  {
    result = rightCard->getResultCardinality();
    return result;
  }

  CANodeIdSet leftSetPredecessors;
  CANodeIdSet newNodes = leftNodesJoinedToRight;
  CANodeIdSet nodesConsidered;

  while(newNodes.entries())
  {
    for( CANodeId lChild = newNodes.init();
         newNodes.next(lChild);
         newNodes.advance(lChild))
    {
      JBBC * lChildJBBC = lChild.getNodeAnalysis()->getJBBC();
      leftSetPredecessors += lChildJBBC->getPredecessorJBBCs();
      nodesConsidered += lChild;
    }

    leftSetPredecessors.intersectSet(leftChildren);
    newNodes = leftSetPredecessors;
    newNodes -= nodesConsidered;
  }

  leftNodesJoinedToRight += leftSetPredecessors;

  // for a JBBSubset to be legal it has to have at least one
  // independent jbbc i.e. a jbbcs connect via a innerNonSemiNonTsjJoin
  // Assumption: leftChildren represents a legal JBBSubset
  CANodeIdSet independentJBBCsInLeftNodesJoinedToRight =
    QueryAnalysis::Instance()->getInnerNonSemiNonTSJJBBCs();

  independentJBBCsInLeftNodesJoinedToRight.intersectSet(leftNodesJoinedToRight);
  
  if(!independentJBBCsInLeftNodesJoinedToRight.entries())
    leftNodesJoinedToRight += 
      leftChildren.jbbcsToJBBSubset()->
        getJBBSubsetAnalysis()->
          getLargestIndependentNode();

  EstLogPropSharedPtr cardLeftNodesJoinedToRight =
    getStatsForCANodeIdSet(leftNodesJoinedToRight);

  // All nodes connected via a join
  CANodeIdSet connectedNodes(leftNodesJoinedToRight);
  connectedNodes += rightChildren;

  EstLogPropSharedPtr cardConnectedNodes =
    joinJBBChildren(leftNodesJoinedToRight,rightChildren);

  result = cardConnectedNodes->getResultCardinality() /
             cardLeftNodesJoinedToRight->getResultCardinality();

  return result;
}

// AppliedStatMan::getStatsForCANodeId is called for applying
// local predicates for the given child. In case the predicates
// are not specified, ASM will apply all local predicates,
// else only the given predicates will be applied. The statistics
// is cached if all local predicates are applied. Partial statistics
// (corresponding to a specific predicate) is re-computed

EstLogPropSharedPtr AppliedStatMan::getStatsForCANodeId(
					CANodeId jbbc,
					const EstLogPropSharedPtr &inLP,
					const ValueIdSet * predIdSet)
{

  EstLogPropSharedPtr inputLP = inLP;

  if(inputLP == (*GLOBAL_EMPTY_INPUT_LOGPROP))
    inputLP = jbbc.getJBBInput();

  EstLogPropSharedPtr outputEstLogProp = NULL;

  // 1. Try to find Logical Properties from cache if cacheable.

  // The estimate Logical Properties can be cacheable if all local
  // predicates are to be applied and if inNodeSet is provided,
  // or the inLP are cacheable

  if ((inputLP->isCacheable()) && (predIdSet == NULL) )
  {
    CANodeIdSet combinedSet = jbbc;

    // get the nodeIdSet of the outer child, if not already given. This
    // along with the present jbbc is used as a key in the cache

    CANodeIdSet * inputNodeSet;
    inputNodeSet = inputLP->getNodeSet();

    // if inLP are cacheable these should have a nodeSet attached
    CCMPASSERT(inputNodeSet != NULL);

    if (inputNodeSet)
    {
      combinedSet.insert(*inputNodeSet);
      // if estLogProp for all local predicates is required,
      // then it might already exist in the cache
      outputEstLogProp = getCachedStatistics(&combinedSet);
    }
  }

  if (outputEstLogProp == NULL)
  {
    // 2. properties do not exist in the cache, so synthesize them.

    //if specified by the user apply those predicates,
    // else apply predicates in the original expr
    NodeAnalysis * jbbcNode = jbbc.getNodeAnalysis();

    TableAnalysis * tableAnalysis = jbbcNode->getTableAnalysis();

    if (tableAnalysis && predIdSet)
    {
      TableDesc * tableDesc = tableAnalysis->getTableDesc();

      const QualifiedName& qualName = 
            tableDesc->getNATable()->getTableName();

      CorrName name(qualName, STMTHEAP);

      Scan *scanExpr = new STMTHEAP Scan(name, tableDesc, REL_SCAN, STMTHEAP);

      Cardinality rc = tableDesc->getNATable()->getEstRowCount();

      const CardinalityHint* cardHint = tableDesc->getCardinalityHint();
      if ( cardHint ) 
         rc = (cardHint->getScanCardinality()).getValue();

      if ( !cardHint && tableDesc->getNATable()->isHbaseTable() ) {

          NATable* nt = (NATable*)(tableDesc->getNATable());
   
          StatsList* statsList = nt->getColStats();
   
          if ( statsList && statsList->entries() > 0 ) {
              ColStatsSharedPtr cStatsPtr = 
                    statsList->getSingleColumnColStats(0);
   
              if ( cStatsPtr )
                 rc = (cStatsPtr->getRowcount()).getValue();
          }
      }

      scanExpr->setBaseCardinality(MIN_ONE (rc));

      GroupAttributes * gaExpr = new STMTHEAP GroupAttributes();

      scanExpr->setSelectionPredicates(*predIdSet);

      ValueIdSet requiredOutputs = jbbc.getNodeAnalysis()->\
	getOriginalExpr()->getGroupAttr()->getCharacteristicOutputs();

      gaExpr->setCharacteristicOutputs(requiredOutputs);

      scanExpr->setGroupAttr(gaExpr);
      gaExpr->setLogExprForSynthesis(scanExpr);

      EstLogPropSharedPtr nonCacheableInLP(new (HISTHEAP) EstLogProp (*inputLP));
      nonCacheableInLP->setCacheableFlag(FALSE);
      scanExpr->synthLogProp();
      outputEstLogProp = scanExpr->getGroupAttr()->outputLogProp(nonCacheableInLP);
    }
    else
    {
        NodeAnalysis * nodeAnalysis = jbbc.getNodeAnalysis();

        RelExpr * relExpr = nodeAnalysis->getModifiedExpr();

	if (relExpr == NULL)
	  relExpr = nodeAnalysis->getOriginalExpr();

      // synthesize and cache estLogProp for the given inLP.
      outputEstLogProp = relExpr->getGroupAttr()->outputLogProp(inputLP);
    }
  }

  return outputEstLogProp;
} // getStatsForCANodeId


// get Stats after applying local predicates to Clustering key columns of JBBC
EstLogPropSharedPtr AppliedStatMan::getStatsForLocalPredsOnCKPOfJBBC(
		      CANodeId jbbc,
		      const EstLogPropSharedPtr &inLP)
{
  EstLogPropSharedPtr inputLP = inLP;

  if(inputLP == (*GLOBAL_EMPTY_INPUT_LOGPROP))
    inputLP = jbbc.getJBBInput();

    // if the Jbbc is not a table return

  TableAnalysis * tableAnalysis = jbbc.getNodeAnalysis()->getTableAnalysis();
  if(tableAnalysis == NULL)
   return getStatsForCANodeId(jbbc, inputLP);

  const ValueIdList &skeys =
      tableAnalysis->getTableDesc()->getClusteringIndex()->getClusteringKeyCols();

  return getStatsForLocalPredsOnPrefixOfColList(jbbc, skeys, inputLP);
} // AppliedStatMan::getStatsForLocalPredsOnCKPOfJBBC

// get Stats after applying local predicates on the given columns of JBBC
// if there are no predicates on the given colum set, return NULL
EstLogPropSharedPtr AppliedStatMan::getStatsForLocalPredsOnPrefixOfColList(
		      CANodeId jbbc,
		      const ValueIdList colIdList,
		      const EstLogPropSharedPtr &inLP)
{

  EstLogPropSharedPtr inputLP = inLP;

  if(inputLP == (*GLOBAL_EMPTY_INPUT_LOGPROP))
    inputLP = jbbc.getJBBInput();

  TableAnalysis * tableAnalysis = jbbc.getNodeAnalysis()->getTableAnalysis();

  if (tableAnalysis == NULL)
  {
    // apply all local predicates, if JBBC is not a table (could be a sub-query)
    return getStatsForCANodeId(jbbc, inputLP);
  }

  ValueIdSet localPredsOnCols;
  Lng32 prefixSize;

  // get local predicates for leading key columns
  localPredsOnCols = tableAnalysis->getLocalPredsOnPrefixOfList(colIdList,
				       prefixSize);

  return getStatsForCANodeId(jbbc, inputLP, &localPredsOnCols);

} // AppliedStatMan::getStatsForLocalPredsOnPrefixOfColList

// get Stats after applying local predicates on the given columns of JBBC
EstLogPropSharedPtr AppliedStatMan::getStatsForLocalPredsOnGivenCols(
		      CANodeId jbbc,
		      const ValueIdSet colIdSet,
		      const EstLogPropSharedPtr &inLP)
{
  EstLogPropSharedPtr inputLP = inLP;

  if(inputLP == (*GLOBAL_EMPTY_INPUT_LOGPROP))
    inputLP = jbbc.getJBBInput();

  TableAnalysis * tableAnalysis = jbbc.getNodeAnalysis()->getTableAnalysis();

  if (tableAnalysis == NULL)
  {
    // if jbbc is not a table, then apply all local predicates
    return getStatsForCANodeId(jbbc, inputLP);
  }

  ValueIdSet localPredsOnCols;

  ValueIdList colList = colIdSet;

  // get local predicates for all key columns
  localPredsOnCols = tableAnalysis->getLocalPredsOnColumns(colList);

  return getStatsForCANodeId(jbbc, inputLP, &localPredsOnCols);
} // AppliedStatMan::getStatsForLocalPredsOnGivenCols

// get Stats after doing a join on the Clustering key columns of JBBC
EstLogPropSharedPtr AppliedStatMan::getStatsForJoinPredsOnCKOfJBBC(
		      const CANodeIdSet jbbSubset,
		      CANodeId jbbc,
		      EstLogPropSharedPtr &inLP)
{
  EstLogPropSharedPtr inputLP = inLP;

  if(inputLP == (*GLOBAL_EMPTY_INPUT_LOGPROP))
    inputLP = jbbc.getJBBInput();

  TableAnalysis * tableAnalysis = jbbc.getNodeAnalysis()->getTableAnalysis();
  if(tableAnalysis == NULL)
   return joinJBBChildren(jbbSubset, jbbc, inputLP);

  const ValueIdList &skeys =
      tableAnalysis->getTableDesc()->getClusteringIndex()->getClusteringKeyCols();

  return getStatsForJoinPredsOnCols(jbbSubset, jbbc, skeys, TRUE, inputLP);
} // AppliedStatMan::getStatsForJoinPredsOnCKOfJBBC

// get Stats after applying given join predicates on the JBBC
EstLogPropSharedPtr AppliedStatMan::getStatsForGivenJoinPredsOnJBBC(
		      const CANodeIdSet jbbSubset,
		      CANodeId jbbc,
		      const ValueIdSet joinPreds,
		      const ValueIdSet localPreds,
		      EstLogPropSharedPtr &inLP)
{
  EstLogPropSharedPtr inputLP = inLP;

  if(inputLP == (*GLOBAL_EMPTY_INPUT_LOGPROP))
    inputLP = jbbc.getJBBInput();

  EstLogPropSharedPtr outputEstLogProp;

  // form a Join expression with the given join predicates, and compute
  // output estimated logical properties. These properties should not be cached
  // in the ASM cache, hence set the "cacheable" flag to FALSE in inLP.
  // We do not want to modify the "cacheable" flag in the inLP, hence make a
  // copy of these logical properties.

  EstLogPropSharedPtr nonCacheableInLP(new (HISTHEAP) EstLogProp (*inputLP));
  nonCacheableInLP->setCacheableFlag(FALSE);

  Join * joinExpr = formJoinExprForJoinOnJBBC(jbbSubset,
						 jbbc,
                 &localPreds,
                 &joinPreds,
                 nonCacheableInLP,
                 FALSE);
                 
  
  // synthesize estimate logical properties for the join
  outputEstLogProp = joinExpr->getGroupAttr()->outputLogProp(nonCacheableInLP);

  return outputEstLogProp;
} // AppliedStatMan::getStatsForGivenJoinPreds

EstLogPropSharedPtr AppliedStatMan::getStatsForJoinPredsOnCols(const CANodeIdSet leftChild,
		      CANodeId rightChild,
		      const ValueIdList keyColList,
		      NABoolean onlyLeadingCols,
		      EstLogPropSharedPtr &inLP)
{

  EstLogPropSharedPtr inputLP = inLP;

  if(inputLP == (*GLOBAL_EMPTY_INPUT_LOGPROP))
    inputLP = rightChild.getJBBInput();

  JBBC * jbbc = rightChild.getNodeAnalysis()->getJBBC();

  // This is Fatal. Somthing went wrong in the Analyzer 
  CMPASSERT (jbbc != NULL);

  // get all local predicates on the right child

  TableAnalysis * tableAnalysis = rightChild.getNodeAnalysis()->getTableAnalysis();

  if(tableAnalysis == NULL)
  {
    // if right child is not a table, then do a regular join between the two children
    return joinJBBChildren(leftChild, rightChild, inputLP);
  }

  // get the local and the join predicates on the given columns. The predicates
  // are computed only for the prefix columns or all columns depending on the
  // flag passed by the user.

  ValueIdSet joinPredsOfCK;
  ValueIdSet leadingColsPreds;
  Lng32 prefixSize;
  CANodeIdSet connectedJBBCs;

  if (onlyLeadingCols)
    connectedJBBCs = tableAnalysis->getJBBCsConnectedToPrefixOfList(leftChild,
                                              keyColList,
                                              prefixSize,
                                              joinPredsOfCK,
                                              leadingColsPreds);
  else
    connectedJBBCs = tableAnalysis->getJBBCsConnectedToCols(leftChild,
                                      keyColList,
                                      joinPredsOfCK,
                                      leadingColsPreds);


  EstLogPropSharedPtr outputLogProp = getStatsForGivenJoinPredsOnJBBC(leftChild,
							rightChild,
							joinPredsOfCK,
							leadingColsPreds,
							inputLP);

  return outputLogProp;

} // AppliedStatMan::getStatsForJoinPredsOnCols

/*****************************************
TO GO IN CLASS QueryAnalysis
******************************************/

void QueryAnalysis::initializeASM()
{
  AppliedStatMan * appStatMan = ASM();

  if (appStatMan == NULL)
    return;

  ARRAY(JBB *) allJBBs = getJBBs();
  CollIndex remainingJBBs = allJBBs.entries();
  for (CollIndex i = 0; remainingJBBs > 0; i++)
  {
    if (allJBBs.used(i))
    {
      appStatMan->setupASMCacheForJBB(*(allJBBs[i]));
	  remainingJBBs--;
    }
  }
}


