// **********************************************************************
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
// **********************************************************************

// ***********************************************************************
//
// File:         MVMemo.cpp
// Description:  
//               
//               
//               
//
// Created:      12/04/07
// ***********************************************************************

#include "QmsMVMemo.h"
#include "QRLogger.h"

//#define DEBUG_MVMEMO

//========================================================================
//  Class MVMemoPhysicalExpression
//========================================================================

MVMemoPhysicalExpression::MVMemoPhysicalExpression(const QRJoinSubGraphMapPtr map, 
			  Int32 groupNumber, 
			  const MVDetailsPtr mv, 
			  ADD_MEMCHECK_ARGS_DEF(CollHeap* heap))
  : MVMemoLogicalExpression(map->getHashKey(), groupNumber, ADD_MEMCHECK_ARGS_PASS(heap))
   ,mv_(mv)
   ,map_(map)
{}

MVMemoPhysicalExpression::~MVMemoPhysicalExpression() 
{
  deletePtr(map_);
}

const NAString& MVMemoPhysicalExpression::getMVName()
{
  return mv_->getMVName();
}

//========================================================================
//  Class MVMemoGroup
//========================================================================


/**
 * Create a new logical expression for the hash key and insert it into the 
 * logical expression list.
 * @param key Hash key of new logical expression.
 * @param heap For allocating the new logical expression object.
 * @return The new logical expression object.
 *****************************************************************************
 */
MVMemoLogicalExpressionPtr MVMemoGroup::addLogicalExpression(const NAString& key, 
							     ADD_MEMCHECK_ARGS_DEF(CollHeap* heap))
{
  MVMemoLogicalExpressionPtr newExpr = new (heap) 
    MVMemoLogicalExpression(key, getGroupNumber(), ADD_MEMCHECK_ARGS_PASS(heap));
  logicalExprs_.insert(newExpr);
  return newExpr;
}

/**
 * Create a new physical expression for the hash key and insert it into the 
 * physical expression list.
 * @param key Hash key of new physical expression.
 * @param mv The MV details 
 * @param hasGroupBy Is this MVMemoGroup representing MVs with a groupby clause?
 * @param heap For allocating the new physical expression object.
 * @return TRUE if a new physical expression was added, or 
 *         FALSE if this is a duplicate.
 *****************************************************************************
 */
NABoolean MVMemoGroup::addPhysicalExpression(const QRJoinSubGraphMapPtr   map, 
					     const MVDetailsPtr	          mv, 
					     const QRJBBPtr		  jbb,
					     NABoolean		          hasGroupby,
					     ADD_MEMCHECK_ARGS_DEF(CollHeap* heap))
{
  const NAString& mvName = mv->getMVName();
  if (physicalExprsHash_.contains(&mvName))
  {
    // One is enough.
    return FALSE;
  }

  MVMemoPhysicalExpressionPtr newExpr = new (heap) 
    MVMemoPhysicalExpression(map, getGroupNumber(), mv, heap);
  physicalExprsHash_.insert(&mvName, newExpr);

  mv->addMVMemoGroup(this);

  // This is a Groupby node - handle the lattice index.
  if (hasGroupby)
  {
   try
   {
    if (groupLattice_ == NULL)
    {
      // This is the first MV in this group. Create a new GroupLattice object.
#ifdef _DEBUG
      // Lower initial max keys in debug mode so existing dev tests will 
      // exercise code for bitmap resizing.
      groupLattice_ = new(heap) QRGroupLattice(heap, ADD_MEMCHECK_ARGS(10));
#else
      groupLattice_ = new(heap) QRGroupLattice(heap, ADD_MEMCHECK_ARGS(50));
#endif
    }

    // Insert the new MV into the lattice index.
    map->setMVDetails(mv);
    groupLattice_->insert(map, jbb);    
   }
   catch (...)
   {
     QRLogger::log(CAT_GRP_LATTCE_INDX, LL_MVQR_FAIL,
       "Caught exception creating group lattice or inserting mv");
     throw;
   }
  }

  return TRUE;
}

/**
 * Add the MVs in this group to the list of candidates so far.
 * @param[out] candidateList List of MV candidates so far 
 * @param subGraph The subGraph of the group.
 * @param jbb The JBB of the query descriptor, for matching the groupby list.
 * @param heap The heap pointer from which to allocate the MVCandidate objects.
 * @param isPreferredMatch If TRUE, set the isPreferredMatch flag of the candidates.
 *****************************************************************************
 */
void MVMemoGroup::getPhysicalExpressions(MVCandidatesForJBBPtr  mvCandidates, 
					 QRJoinSubGraphMapPtr   querySubGraphMap,
					 QRJBBPtr		jbb,
					 CollHeap*		heap, 
					 NABoolean		isPreferredMatch,
                                         ElementPtrList*        minimizedGroupingList)
{
  if (groupLattice_ != NULL)
  {
   try //@ZXtry
   {
    // This is a groupby node - get the MVs from the lattice index.
    MVCandidatesForJBBSubsetPtr jbbSubset = 
      groupLattice_->search(jbb, mvCandidates, querySubGraphMap, minimizedGroupingList);

    if (jbbSubset)
    {
      // The MVCandidates in this JbbSubset still don't have the MV subGraphMap set.
      // For each MV in the jbbSubset, find the corresponding subGraphMap in the 
      // PhysicalExpression, by MV name.
      CollIndex maxEntries = jbbSubset->entries();
      for (CollIndex i=0; i<maxEntries; i++)
      {
        MVCandidatePtr mv = (*jbbSubset)[i];
        const NAString& mvName = mv->getMvDetails()->getMVName();
        QRJoinSubGraphMapPtr mvSubGraphMap = getSubGraphMapForMV(mvName); 
        mv->setMvSubGraphMap(mvSubGraphMap);
      }

      mvCandidates->insert(jbbSubset);
    }
   }
   catch (...)
   {
     QRLogger::log(CAT_GRP_LATTCE_INDX, LL_MVQR_FAIL,
       "Caught exception searching group lattice or setting subgraph maps");
     throw;
   }
  }
  else
  {
    if (physicalExprsHash_.entries() == 0)
      return;

    MVCandidatesForJBBSubsetPtr jbbSubset = new(heap) 
      MVCandidatesForJBBSubset(mvCandidates, ADD_MEMCHECK_ARGS(heap));
    jbbSubset->setSubGraphMap(querySubGraphMap);

    // No Groupby - get the MVs from the physical expressions.
    // For each physical expression in this group.
    GroupExpressionHashIterator iterator(physicalExprsHash_);
    MVMemoPhysicalExpressionPtr expr = NULL;
    const NAString* mvName = NULL;
    for (CollIndex j = 0; j < iterator.entries(); j++) 
    {
      // Get the MV details
      iterator.getNext(mvName, expr); 
      MVDetailsPtr mv = expr->getMVDetails();
      QRLogger::log(CAT_MVMEMO_JOINGRAPH, LL_DEBUG,
        "Found MV: %s!", mvName->data() );

      // Insert into the result list.
      jbbSubset->insert(mv, jbb, isPreferredMatch, NULL, expr->getMvSubGraphMap(), heap);
    }

    mvCandidates->insert(jbbSubset);
  }
}  // MVMemoGroup::getPhysicalExpressions()

/**
 * Remove a physical expression for an MV that has been dropped.
 * @param key The hash key for the MV to remove.
 *****************************************************************************
 */
void MVMemoGroup::removePhysicalExpression(MVMemoPhysicalExpressionPtr expr)
{
  const NAString& mvName = expr->getMVDetails()->getMVName();
  physicalExprsHash_.remove(&mvName);

  if (groupLattice_ != NULL)
  {
   try //@ZXtry
   {
    groupLattice_->remove(expr->getMvSubGraphMap());
   }
   catch (...)
   {
     QRLogger::log(CAT_GRP_LATTCE_INDX, LL_MVQR_FAIL,
       "Caught exception removing entry from group lattice");
     throw;
   }
  }
}

/**
 * 
 * @param name 
 * @return 
 *****************************************************************************
 */
void MVMemoGroup::dropMV(const NAString& name)
{
  MVMemoPhysicalExpressionPtr expr = physicalExprsHash_.getFirstValue(&name);
  if (expr != NULL)
  {
    if (groupLattice_)
    {
     try //@ZXtry
     {
      groupLattice_->remove(expr->getMvSubGraphMap());
     }
     catch (...)
     {
       QRLogger::log(CAT_GRP_LATTCE_INDX, LL_MVQR_FAIL,
         "Caught exception removing mvDetails from group lattice");
       throw;
     }
    }

    physicalExprsHash_.remove(&name);
    deletePtr(expr);
  }
}

/**
 * An MV is being renamed. The MV name is the key of the physicalExprsHash_
 * hash table. Start the rename operation by removing the MV from the hash 
 * table when the MVDetails object inside it still holds the old name.
 * Keep a pointer to the physical expression object in a temp data member.
 *****************************************************************************
 */
void MVMemoGroup::startRenameMV(const NAString& oldName)
{
  MVMemoPhysicalExpressionPtr expr = physicalExprsHash_.getFirstValue(&oldName);
  assertLogAndThrow(CAT_MVMEMO_JOINGRAPH, LL_MVQR_FAIL, 
                    expr != NULL, QRLogicException, 
		    "Can't find the MV to rename in the MVMemoGroup.");

  physicalExprsHash_.remove(&oldName);
  tempExpr_ = expr;
}

/**
 * An MV is being renamed. The MV name is the key of the physicalExprsHash_
 * hash table. Finish the rename operation by re-inserting the physical 
 * expression when the MVDetails object inside it holds the new name.
 *****************************************************************************
 */
void MVMemoGroup::FinishRenameMV()
{
  physicalExprsHash_.insert(&tempExpr_->getMVName(), tempExpr_);
  tempExpr_ = NULL;
}

Int32 MVMemoGroup::getNumOfMVs()
{
  return physicalExprsHash_.entries();
}

/**
 * This method can be used to dump detailed usage stats to the log file,
 * including MVMemo hash keys and LatticeIndex graphs. 
 * Currently not used.
 *****************************************************************************
 */
void MVMemoGroup::reportStats(NAString& text)
{
  text += "\n== These MVs:";

  GroupExpressionHashIterator iterator(physicalExprsHash_);
  MVMemoPhysicalExpressionPtr expr = NULL;
  const NAString* mvName = NULL;
  for (CollIndex j = 0; j < iterator.entries(); j++) 
  {
    // Get the MV details
    iterator.getNext(mvName, expr); 
    text += " ";
    text += *mvName;
  }

  text += "\nUse this key:\n";
  const NAString& hashKey = expr->getHashKey();
  text += hashKey;

  if (groupLattice_)
  {
    groupLattice_->reportStats(text);
  }
}

/**
 * Create a ProposedMV object and add to it all the MVs that are in physical
 * expressions in this group.
 * If this group has a GroupBy, cascade the work to the LatticeIndex.
 *****************************************************************************
 */
void MVMemoGroup::collectMVGroups(WorkloadAnalysisPtr workload, Int32 minQueriesPerMV, CollHeap* heap)
{
  if (physicalExprsHash_.entries() == 0)
    return;

  // Iterate over all the physical expressions.
  // In case of a GroupBy, we don't really need the iterator, 
  // but we do need to get the pointer to the subgraph map
  // from the first element.
  GroupExpressionHashIterator iterator(physicalExprsHash_);
  MVMemoPhysicalExpressionPtr physExpr = NULL;
  const NAString* mvName = NULL;
  iterator.getNext(mvName, physExpr); 

  if (groupLattice_)
  {
    // The LatticeIndex will create a ProposedMV object for each node
    // with a different GroupBy list.
    groupLattice_->collectMVGroups(workload, minQueriesPerMV, heap);
  }
  else
  {
    if (physicalExprsHash_.entries() >= minQueriesPerMV)
    {
      // No GroupBy - so add all the MVs to the proposedMV objects.
      ProposedMVPtr proposedMV = new(heap) ProposedMV(ADD_MEMCHECK_ARGS(heap));
      do
      {
	// Get the MV details
	proposedMV->addMV(physExpr->getMVDetails(), physExpr->getMvSubGraphMap());
	iterator.getNext(mvName, physExpr); 
      } while (physExpr != NULL);
    }
  }
}


//========================================================================
//  Class MVMemo
//========================================================================

MVMemo::~MVMemo()
{
  // Delete the groups in the groups array.
  for (CollIndex i=0; i<groupsArray_.entries(); i++)
  {
    MVMemoGroupPtr group = groupsArray_[i];
    deletePtr(group);
  }
  groupsArray_.clear();

  // The group hash hash pointers to the same groups.
  expressionHash_.clear();
}

/**
 * Search algorithm of a query in MVMemo.\n
 * -  Create a HubIterator object for insert operations on the hub of the MV.\n
 * -  For each subexpression hashKey produced by the HubIterator:\n
 *   -  Lookup hashKey in the MVMEMO hash table.\n
 *   -  If an expression was found:\n
 *     -  If hashKey represents a base table, and exp is a physical 
 *        expression for an MV
 *       -  Provide the MV information to the HubIterator, so it will 
 *          rewrite its own join graph.\n
 *     -  Else (an expression exp was not found so no corresponding group exists)\n
 *       -  Create an empty group g.\n
 *       -  Create a logical expression on the hash key and add it to the group g.\n
 *       -  Insert the logical expression into the MVMEMO hash table.\n
 *       -  If the hashKey involves a self join\n
 *         -  For each equivalent hash key from the HubIterator\n
 *           -  Create a new logical expression for the new hash key\n
 *           -  Add the logical expression into group g.\n
 *           -  Insert the logical expression into the MVMEMO hash table.\n
 * -  With g as the last group found or inserted, representing the top
 *    of the hub, if g has any matching MVs, mark them as preferred matches.\n
 *
 * @param jbb The JBB from the MV descriptor to insert into MVMemo.
 * @param mv The details of the MV being inserted.
 *****************************************************************************
 */
void MVMemo::insert(QRJBBPtr jbb, MVDetailsPtr mv)
{
  QRJoinSubGraphPtr subGraph = NULL;

  const QRMVMiscPtr miscElement = mv->getMvDescriptor()->getMisc();
  NABoolean isWorkloadAnalysisMode = miscElement != NULL && miscElement->isFromQuery();
  OperationType op = OP_INSERT;
  // Insert only the top, full join graph if this is a MAV/MAJV or if
  // we are in workload analysis mode.
  if (mv->hasGroupBy()     ||
      isWorkloadAnalysisMode )
    op = OP_INSERT_TOP;

  // Create a HubIterator for the JBB
  HubIteratorPtr hubIterator = createHubIteratorForJBB(jbb, mv, op, heap_);
  // Heuristic method to check if self join processing should be skipped.
  NABoolean isSelfJoinTooBig = hubIterator->isSelfJoinTooBig();

  do // For each subgraph produced by the HubIterator
  {
    subGraph = hubIterator->nextSubGraph();

    if (subGraph == NULL)
      break;

    QRJoinSubGraphMapPtr  map = subGraph->getSubGraphMap(op);

    NABoolean isDuplicate = FALSE;
    NABoolean found = handleInsertedSubgraph(map, jbb, mv, isDuplicate);
    if (isDuplicate)
    {
      deletePtr(map);
      map = NULL;
    }

    // Do self-join processing only if:
    // 1. This is a self-join graph.
    // 2. The self-join is not too big.
    // 3. An MV with a duplicate hash key was not already inserted.
    // 3. This expression was not found in MVMemo (so we did not already 
    //    go through this process) or this is the top expression 
    //    of the join graph (so we need to insert the MV in all the groups).
    if ( subGraph->isSelfJoin() && 
        !isSelfJoinTooBig       && 
        !isDuplicate            &&
	(!found || (NULL != map && map->isFull())) )
    {
      // Generate all the equivalent hash keys, and insert them all.
      map->prepareForSelfJoinWork();
      while (map->hasNextEquivalentMap())
      {
        QRJoinSubGraphMapPtr equivalentMap = map->nextEquivalentMap();
	if (equivalentMap == NULL)
	  break;

        handleInsertedSubgraph(equivalentMap, jbb, mv, isDuplicate);
        if (isDuplicate)
        {
          deletePtr(equivalentMap);
          equivalentMap = NULL;
        }
      }
    }

    if (map && (!map->isFull()))
    {
      // The map is only needed later if we inserted a physical expression.
      deletePtr(map);
      map = NULL;
    }

  } while (hubIterator->hasNext());

  deletePtr(hubIterator);
}  // MVMemo::insert()

/**
 * Search algorithm of query matches in MVMemo.\n
 * -  Create a HubIterator object for search operations on the hub of the MV.\n
 * -  For each subexpression hashKey produced by the HubIterator:\n
 *   -  Lookup hashKey in the MVMEMO hash table.\n
 *   -  If an expression was not found:\n
 *     -  If hashKey represents a base table, and exp is a physical 
 *        expression for an MV, provide the MV information to the 
 *  	  HubIterator, so it will rewrite its own join graph.\n
 *     -  If exp is a logical expression, and the corresponding 
 *	  group includes physical expressions for MVs, add those
 *	  MVs to the candidate list.\n
 *   -  Else (an expression exp was found)\n
 *     -  Call HubIterator to report this subexpression as missing, 
 *        so no supersets of it will be attempted.\n
 *   -  With g as the last group found or inserted, representing the top 
 *      of the hub, if g has any matching MVs, mark them as preferred matches.
 *
 * @param jbb The JBB from the query descriptor, to search for MVs matching it.
 * @param[out] candidateList List of MV candidates so far 
 * @param heap The request heap.
 *****************************************************************************
 */
void MVMemo::search(QRJBBPtr jbb, MVCandidatesForJBBPtr mvCandidates, CollHeap* heap)
{
  QRLogger::log(CAT_MVMEMO_JOINGRAPH, LL_DEBUG,
    "==========>>> Matching JBB: %s.", jbb->getID().data());

  // Create a HubIterator for the JBB
  HubIteratorPtr hubIterator = createHubIteratorForJBB(jbb, NULL, OP_SEARCH, heap);

  // Before we start the iteration, check for the IndirectGroupBy case
  // Are there any tables we can reduce because they are joined on a unique key?
  const QRJoinSubGraphPtr mini = hubIterator->getMinimizedSubGraph();
  if (mini != NULL)
  {
    // OK, are there any MVs that correspond to the minimized subgraph?
    const QRJoinSubGraphMapPtr map = mini->getSubGraphMap(OP_SEARCH);
    MVMemoGroupPtr group = probeForSubGraph(hubIterator, map);
    if (group != NULL)
    {
      // Transform the grouping list to use only fact table columns.
      GroupingListMinimizer minimizer(jbb, map, ADD_MEMCHECK_ARGS(heap_));
      ElementPtrList* minimizedGroupingList = minimizer.calcIndirectGroupingList();      
      if (minimizedGroupingList == NULL)
      {
        // Could not minimize grouping list - cleanup.
        deletePtr(map);
      }
      else
      {
        // Use the minimized grouping list to probe the GroupLattice for matching MVs.
        group->getPhysicalExpressions(mvCandidates, map, jbb, heap, FALSE, minimizedGroupingList);
      }
    }
  }

  do // For each subexpression hashKey produced by the HubIterator
  {
    const QRJoinSubGraphPtr subGraph = hubIterator->nextSubGraph();
    if (subGraph == NULL)
      break;

    const QRJoinSubGraphMapPtr  map = subGraph->getSubGraphMap(OP_SEARCH);
    QRLogger::log(CAT_MVMEMO_JOINGRAPH, LL_DEBUG,
      "Probing for hash key: \n%s", map->getHashKey().data());

    MVMemoGroupPtr group = probeForSubGraph(hubIterator, map);
    if (group != NULL)
    {
      // Was this the last subexpression?
      NABoolean isTopOfQuery = (FALSE == hubIterator->hasNext());

      // Add them to the list of candidates so far.
      group->getPhysicalExpressions(mvCandidates, map, jbb, heap, isTopOfQuery, NULL);
    }
  } while (hubIterator->hasNext());

  deletePtr(hubIterator);

}  // MVMemo::search()

/**
 * Analyze a JBB and create a HubIterator for it. This method is used both 
 * when inserting an MV, as well as when matching a query.
 * @param jbb   The JBB definition from the MV  or Query Descriptor.
 * @param op    Is this an insert or a search operation?
 * @param heap  The heap from which to allocate objects. This should be the 
 *              main heap for MVs, and the request heap for queries.
 *****************************************************************************
 */
HubIteratorPtr MVMemo::createHubIteratorForJBB(const QRJBBPtr jbb, 
					       MVDetailsPtr   mv, 
					       OperationType  op, 
					       CollHeap*      heap)
{
  // Create a new join graph for this JBB.
  QRJoinGraphPtr joinGraph = new (heap) QRJoinGraph(heap); 

  joinGraph->initFromJBB(jbb, mv);

  HubIteratorPtr iter = new(heap) HubIterator(joinGraph, op, ADD_MEMCHECK_ARGS(heap));
  iter->init();
  return iter;
}

/**
 * Insert a new logical expression for a new subgraph.
 * If the subgraph is for the full joingraph, add a physical expression as well.
 * This version of the method creates a new group for each equivalent self-join 
 * logical expression. The version with the 'group' parameter, puts all the 
 * equivalent logical expressions in the same group. This is the way its 
 * described in the IS doc, and is probably better, but it still has a bug
 * where the MV is disqualified when matched.
 * @param map The subGraphMap to be inserted.
 * @param jbb The JBB element of the descriptor
 * @param mv The MVDetails of the MV being inserted.
 * @return TRUE if hash key was found in MVMemo, FALSE if inserted a new group.
 *****************************************************************************
 */
NABoolean MVMemo::handleInsertedSubgraph(QRJoinSubGraphMapPtr map, 
                                         QRJBBPtr jbb, 
                                         MVDetailsPtr mv,
                                         NABoolean& isDuplicate)
{
  isDuplicate = FALSE;
  const NAString& key = map->getHashKey();
  MVMemoGroupPtr group = NULL;
  QRLogger::log(CAT_MVMEMO_JOINGRAPH, LL_DEBUG,
    "Checking hash key: \n%s", key.data());

  NABoolean found = contains(key);
  if (found) // Was the expression found in the hash table?
  {
    // Yes - The expression hash key was found.
    MVMemoLogicalExpressionPtr expr = getExpression(key);
    Int32 groupNumber = expr->getGroupNumber();
    group = getGroup(groupNumber);

    // Found an existing logical expression - ignore.
    QRLogger::log(CAT_MVMEMO_JOINGRAPH, LL_DEBUG,
      "  Found existing group %d.\n", groupNumber);
  }
  else
  {
    // No - the expression hash key was not found in the hash table.
    if (group==NULL)
    {
      // Create a new group for it.
      group = allocateNewGroup();
    }

    // Create a new logical expression and add it to the group and to the hash table.
    MVMemoLogicalExpressionPtr newExpr = 
      group->addLogicalExpression(key, ADD_MEMCHECK_ARGS(heap_));
    insertExpression(newExpr);

    QRLogger::log(CAT_MVMEMO_JOINGRAPH, LL_DEBUG,
      "  Inserted hash key in new group %d.\n", group->getGroupNumber());
  } // else - group not found

  if (map->isFull())
  {
    // This hash key is for the full MV - insert a physical expression.
    isDuplicate = !group->addPhysicalExpression(map, mv, jbb, map->hasGroupBy(), ADD_MEMCHECK_ARGS(heap_));

    QRLogger::log(CAT_MVMEMO_JOINGRAPH, LL_DEBUG,
      "  Inserted the MV in group %d.\n", group->getGroupNumber());
  }

  return found;
}  // MVMemo::handleInsertedSubgraph()


/**
 *
 *****************************************************************************
 */
MVMemoGroupPtr MVMemo::probeForSubGraph(HubIteratorPtr hubIterator, const QRJoinSubGraphMapPtr map)
{
  const NAString& key = map->getHashKey();

  if(contains(key)) // Was the expression found in the hash table?
  {
    // Yes - The expression hash key was found.
    MVMemoLogicalExpressionPtr expr = getExpression(key);
    if (expr->isPhysical()) 
    {
      // The expression is not for a table - its for an MV.
      // Use the MV's information to rewrite the query join graph, so
      // that it will use base tables rather than the MV.
      MVMemoPhysicalExpressionPtr physicalExpr = static_cast<MVMemoPhysicalExpressionPtr>(expr);
      hubIterator->rewriteJoinGraph(physicalExpr->getMVDetails());
    }
    else
    {
      // Are there any MVs in this group?
      MVMemoGroupPtr group = getGroup(expr->getGroupNumber());

      QRLogger::log(CAT_MVMEMO_JOINGRAPH, LL_DEBUG,
        "Found group no. %d.\n", group->getGroupNumber());

      if (group->hasPhysicalExpressions())
      {
        return group;
      }
    }
  }
  else
  {
//debugMessage1("Didn't find group for: %s", key.data());
    // No - the expression hash key was not found in the hash table.
    hubIterator->reportAsMissing();
  }

  // Delete the map object only if its not used in any MV candidates.
  deletePtr(map);
  return NULL;
}

/**
 * This method can be used to dump detailed usage stats to the log file,
 * including MVMemo hash keys and LatticeIndex graphs. 
 * Currently not used.
 *****************************************************************************
 */
void MVMemo::reportStats(Int32 numOfMVs)
{
  typedef NAPtrList<MVMemoGroupPtr> GroupList;
  NAArray<GroupList*>  groupsOrderedArray(heap_, numOfMVs);
  MVMemoGroupPtr currentGroup=NULL;
  CollIndex currentGroupSize=0;
  GroupList* currentGroupList = NULL;

  // Sort the groups into an array of lists according to the group size.
  CollIndex maxEntries=groupsArray_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    currentGroup = groupsArray_[i];
    currentGroupSize = currentGroup->getNumOfMVs();
    if (currentGroupSize == 0)
      continue;

    if (groupsOrderedArray.used(currentGroupSize))
      currentGroupList = groupsOrderedArray[currentGroupSize];
    else
    {
      currentGroupList = new(heap_) GroupList(heap_);
      groupsOrderedArray.insertAt(currentGroupSize, currentGroupList);
    }
    currentGroupList->insert(currentGroup);
  }

  // Dump the stats to the log.
  NAString text("\n", heap_);
  char buff[100];
  text += "=============================\n";
  text += "== MVMemo Usage Statistics ==\n";
  text += "=============================\n";
  QRLogger::log(CAT_MVMEMO_STATS, LL_DEBUG, text.data());
  text = "";

  CollIndex groupsProcessed = 0;
  maxEntries = groupsOrderedArray.entries();
  for (currentGroupSize=0; groupsProcessed<maxEntries; currentGroupSize++)
  {
    if (!groupsOrderedArray.used(currentGroupSize))
      continue;

    sprintf(buff, "\n===== MVMemoGroups of size: %d\n", currentGroupSize);
    QRLogger::log(CAT_MVMEMO_STATS, LL_DEBUG, buff);

    currentGroupList = groupsOrderedArray[currentGroupSize];
    CollIndex maxGroups = currentGroupList->entries();
    for (CollIndex j=0; j<maxGroups; j++)
    {
      currentGroup = (*currentGroupList)[j];
      currentGroup->reportStats(text);
    }

    QRLogger::log(CAT_MVMEMO_STATS, LL_DEBUG, text.data());
    text = "";
  
    // Cleanup
    delete currentGroupList;
    groupsOrderedArray[currentGroupSize] = NULL;

    groupsProcessed++;
  }
}

/**
 * Iterate over all the MVMemo groups to collect proposed MVs.
 *****************************************************************************
 */
void MVMemo::collectMVGroups(WorkloadAnalysisPtr workload, Int32 minQueriesPerMV, CollHeap* heap)
{
  CollIndex maxEntries=groupsArray_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    groupsArray_[i]->collectMVGroups(workload, minQueriesPerMV, heap);
  }
}

//========================================================================
//  Class GroupingListMinimizer
//========================================================================

/**
 * GroupingListMinimizer class constructor
 *****************************************************************************
 */
GroupingListMinimizer::GroupingListMinimizer(const QRJBBPtr jbb, 
                                             const QRJoinSubGraphMapPtr map,
                                             ADD_MEMCHECK_ARGS_DEF(CollHeap* heap))
  : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
    ,heap_(heap)
    ,partitionedGroupingList_(hashKey, INIT_HASH_SIZE_SMALL, TRUE, heap) // Pass NAString::hashKey
    ,fullGroupingList_(jbb->getGroupBy()->getPrimaryList()->getList())
    ,tablesToReduce_(map->getSubGraph()->getParentGraph()->getTablesToReduce())
    ,outputs_(jbb->getOutputList())
{
}

/**
 * Verify that none of the query aggregate functions use columns from
 * reduced tables.
 *****************************************************************************
 */
NABoolean GroupingListMinimizer::verifyAggregateFunctions()
{
  // For each element in the query output list
  CollIndex maxEntries = outputs_->entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    QROutputPtr output = (*outputs_)[i];
    QRElementPtr elem  = output->getOutputItem();
    // If its an expression
    if (elem->getElementType() == ET_Expr)
    {
      QRExprPtr expr = elem->downCastToQRExpr();
      QRExplicitExprPtr expl  = expr->getExprRoot();
      // If its an aggregate function
      if (expl->containsAnAggregate(heap_))
      {
        const ElementPtrList& inputs = expr->getInputColumns(heap_);
        // Check all the expression input columns
        CollIndex maxEntries2 = inputs.entries();
        for (CollIndex j=0; j<maxEntries2; j++)
        {
          QRElementPtr inputElem = inputs[j]->getReferencedElement();
          if (inputElem->getElementType() == ET_Column)
          {
            QRColumnPtr col = inputElem->downCastToQRColumn();
            const NAString& tableID = col->getTableID();

            // Is the input column from a reduced table?
            if (isReducedTable(tableID))
            {
              QRLogger::log(CAT_MVMEMO_JOINGRAPH, LL_DEBUG,
                "Output %s is an aggregate function on an Indirect roupBy reduced table.",
                            output->getID().data());
              return FALSE;
            }
          }
        }
      }
    }
  }

  return TRUE;
}

/**
 *
 *****************************************************************************
 */
NABoolean GroupingListMinimizer::isReducedTable(const NAString& tableID)
{
  CollIndex maxEntries = tablesToReduce_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    if (tablesToReduce_[i]->getID() == tableID)
      return TRUE;
  }

  return FALSE;
}

/**
 * Add a column to the grouping column list.
 * Ignore duplicates.
 *****************************************************************************
 */
void GroupingListMinimizer::addGroupingCol(QRElementPtr groupingCol)
{
  const NAString& tableID = groupingCol->downCastToQRColumn()->getTableID();
  ElementPtrList* tableCols = NULL;
  // Find the grouping list columns from this table.
  if (partitionedGroupingList_.contains(&tableID))
  {
    tableCols = partitionedGroupingList_.getFirstValue(&tableID);
  }
  else
  {
    tableCols = new (heap_) ElementPtrList(heap_);
    partitionedGroupingList_.insert(&tableID, tableCols);
  }

  // Is this column already on the grouping list?
  const NAString& colID = groupingCol->getID();
  CollIndex maxEntries = tableCols->entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    if ((*tableCols)[i]->getID() == colID)
    {
      // Yes, this is a duplicate - just ignore it.
      return;
    }
  }

  // Not a duplicate - add it to the list.
  tableCols->insert(groupingCol);
}

/**
 * Populate the partitionedGroupingList_ data structure. 
 *****************************************************************************
 */
void GroupingListMinimizer::partitionGroupingList()
{
  // For each element in the grouping list.
  CollIndex maxEntries = fullGroupingList_.entries();
  QRColumnPtr groupingCol = NULL;
  for (CollIndex i=0; i<maxEntries; i++)
  {
    QRElementPtr groupingElem = fullGroupingList_[i]->getReferencedElement();
    if (groupingElem->getElementType() == ET_JoinPred)
    {
      // This grouping column is part of an equality set.
      // Use the first column in the equality set.
      QRJoinPredPtr jp = groupingElem->downCastToQRJoinPred();
      const ElementPtrList& eqList = jp->getEqualityList();
      if (eqList[0]->getElementType() == ET_Column)
      {
        QRElementPtr colRefedElem;
        colRefedElem = eqList[0]->getReferencedElement();
        groupingCol = colRefedElem->downCastToQRColumn();
      }
      else
      {
        assertLogAndThrow(CAT_MVMEMO_JOINGRAPH, LL_MVQR_FAIL,
                          groupingElem->getElementType() == ET_Column, QRLogicException,
                          "Grouping by join preds on expressions not yet supported.");
      }
    }
    else
    {
      assertLogAndThrow(CAT_MVMEMO_JOINGRAPH, LL_MVQR_FAIL,
                        groupingElem->getElementType() == ET_Column, QRLogicException,
                        "Grouping by expressions not yet supported.");
      groupingCol = groupingElem->downCastToQRColumn();
    }

    // Add the grouping column to the partitionedGroupingList_.
    addGroupingCol(groupingCol);
  }   
}

/**
 * Reduce a table by translating grouping columns from it to the foreign
 * key columns used to join to this table.
 *****************************************************************************
 */
void GroupingListMinimizer::reduceTable(JoinGraphTablePtr table)
{
  // First get the list of foreign key columns from the joining table,
  // and reduce them from the join graph.
  ElementPtrList pointingCols(heap_);
  table->getAndReducePredicateColumnsPointingToMe(pointingCols);

  // Next check if this table has any grouping columns at all.
  const NAString& tableID = table->getID();
  ElementPtrList* tableCols = partitionedGroupingList_.getFirstValue(&tableID);
  if (tableCols == NULL)
    return;

  // Add them to the partitionedGroupingList_
  CollIndex maxEntries = pointingCols.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    QRElementPtr newGroupingCol = pointingCols[i];
    addGroupingCol(newGroupingCol);
  }

  // Remove this table from the partitionedGroupingList_.
  partitionedGroupingList_.remove(&tableID);
  delete tableCols;

  QRLogger::log(CAT_MVMEMO_JOINGRAPH, LL_DEBUG,
    "Table %s was reduced.", table->getName().data());
}

/**
 * After all the tables in tablesToReduce_ were reduced, make sure none of
 * them were added back by reducing some other table.
 *****************************************************************************
 */
NABoolean GroupingListMinimizer::verifyTablesWereReduced()
{
  CollIndex maxEntries = tablesToReduce_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    JoinGraphTablePtr table = tablesToReduce_[i];
    const NAString& tableID = table->getID();
    if (partitionedGroupingList_.contains(&tableID))
    {
      QRLogger::log(CAT_MVMEMO_JOINGRAPH, LL_DEBUG,
        "Table %s was reduced and then re-added. Aborting Indirect GroupBy algorithm.",
                    table->getName().data() );
      return FALSE;
    }
  }

  return TRUE;
}

/**
 * Prepare the result list of grouping columns.
 *****************************************************************************
 */
ElementPtrList* GroupingListMinimizer::prepareResult()
{
  ElementPtrList* result = new(heap_) ElementPtrList(heap_);

  // Iterate on the partitionedGroupingList_ hash table.
  PartitionedGroupingListIterator iter(partitionedGroupingList_);
  const NAString* key;
  ElementPtrList* value;
  CollIndex maxEntries = iter.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    iter.getNext(key, value); 
    result->insert(*value);
  }

  return result;
}

/**
 * Log a message with the resulting grouping list.
 *****************************************************************************
 */
void GroupingListMinimizer::dumpResult(ElementPtrList* result)
{
  QRLogger::log(CAT_MVMEMO_JOINGRAPH, LL_DEBUG,
    "All dimension tables were successfully reduced!");

  NAString msg(heap_);
  msg = "New Grouping list is: ";

  CollIndex maxEntries = result->entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    if (i!=0)
      msg += ", ";

    msg += (*result)[i]->getID();
  }
  msg += ".";

  QRLogger::log(CAT_MVMEMO_JOINGRAPH, LL_DEBUG, msg);
    
}

/**
 * Run the Indirect GroupBy minimization algorithm.
 *****************************************************************************
 */
ElementPtrList* GroupingListMinimizer::calcIndirectGroupingList()
{
  // First verify that there are no aggregate functions on dimension table columns.
  if (verifyAggregateFunctions() == FALSE)
    return NULL;

  // Populate the main data structure.
  partitionGroupingList();

  // Create a local copy of the table list.
  JoinGraphTableList tablesToReduce(tablesToReduce_);

  // Keep reducing tables as long as tables are being reduced.
  NABoolean tableWasReduced = FALSE;
  do 
  {
    tableWasReduced = FALSE;
    // Do loop backwards because we will be deleting tables we reduce.
    for (Int32 i=tablesToReduce.entries()-1; i>=0; i--)
    {
      JoinGraphTablePtr table = tablesToReduce[i];
      if (table->isOnTheEdge() == FALSE)
        continue;

      reduceTable(table);
      tableWasReduced = TRUE;
      tablesToReduce.remove(table);
    }
  } while (tableWasReduced);

  if (tablesToReduce.entries() > 0)
  {
    // Not all the tables in the list were reduced. Abort.
    QRLogger::log(CAT_MVMEMO_JOINGRAPH, LL_DEBUG,
      "Table %s could not be reduced.", tablesToReduce[0]->getName().data() );
    return NULL;
  }

  // Check that partitionedGroupingList_ does not contain any of the tables in tablesToReduce_
  if (verifyTablesWereReduced() == FALSE)
    return NULL;

  // Create the new grouping list.
  ElementPtrList* result = prepareResult();
  dumpResult(result);

  return result;
}
