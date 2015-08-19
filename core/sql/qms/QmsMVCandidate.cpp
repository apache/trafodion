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
// File:         MVCandidate.cpp
// Description:  
//               
//               
//               
//
// Created:      07/20/08
// ***********************************************************************

#include "QmsMVCandidate.h"
#include "QRDescriptor.h"
#include "QRMessage.h"
#include "ComCextdecs.h"  // For NA_JulianTimestamp()

using namespace QR;

//========================================================================
//  Class MVCandidate
//========================================================================

/****************************************************************************/
MVCandidate::MVCandidate(const MVDetailsPtr	      mvDetails, 
			 const DescriptorDetailsPtr   query,
			 const QRJBBPtr		      jbb,
			 MVCandidatesForJBBSubsetPtr  jbbSubset,
			 ADD_MEMCHECK_ARGS_DEF(CollHeap* heap))
  : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
    ,heap_(heap)
    ,mvDetails_(mvDetails)
    ,queryDetails_(query)
    ,queryJbb_(jbb)
    ,queryJbbDetails_(NULL)
    ,jbbSubset_(jbbSubset)
    ,mvSubGraphMap_(NULL)
    ,extraGroupingColumns_(NULL)
    ,isPreferredMatch_(FALSE)
    ,outputMatching_(ADD_MEMCHECK_ARGS(heap))
    ,rangeMatching_(ADD_MEMCHECK_ARGS(heap))
    ,residualMatching_(ADD_MEMCHECK_ARGS(heap))
    ,groupingMatching_(ADD_MEMCHECK_ARGS(heap))
    ,joinPredMatching_(ADD_MEMCHECK_ARGS(heap))
    ,lojMatching_(ADD_MEMCHECK_ARGS(heap))
    ,matchingType_(ANT_NO_INIT)
    ,extraHubTables_(heap)
    ,backJoinTables_(heap)
    ,wasDisqualified_(FALSE)
    ,disqualifiedReason_(NULL)
    ,tableIDCache_(hashKey, INIT_HASH_SIZE_SMALL, FALSE, heap)
{ 
  if (jbbSubset)
    jbbSubset->registerCandidate(this);

  outputMatching_.setCandidate(this);
  rangeMatching_.setCandidate(this);
  residualMatching_.setCandidate(this);
  groupingMatching_.setCandidate(this);
  joinPredMatching_.setCandidate(this);
  lojMatching_.setCandidate(this);
}

/****************************************************************************/
/****************************************************************************/
QRJoinSubGraphMapPtr MVCandidate::getMvSubGraphMap()
{
  return mvSubGraphMap_;
}

/****************************************************************************/
/****************************************************************************/
QRJoinSubGraphMapPtr MVCandidate::getQuerySubGraphMap()
{
  return jbbSubset_->getQuerySubGraphMap();
}

/****************************************************************************/
// Initialize internal data structures, such as the AggregateMatchingType.
/****************************************************************************/
void MVCandidate::init(NABoolean isPreferredMatch, GroupingList* groupingList)
{
  isPreferredMatch_ = isPreferredMatch;
  extraGroupingColumns_ = groupingList;

  initAggregateMatchingType();
}

/****************************************************************************/
/****************************************************************************/
void MVCandidate::initAggregateMatchingType()
{
  // Check if the query JBB has a GroupBy in it.
  NABoolean QueryHasGroupBy = (queryJbb_->getGroupBy() != NULL);

  if (mvDetails_->hasGroupBy())
  {
    // The MV is a MAV.
    if (QueryHasGroupBy)
    {
      // Both have a GroupBy, check if it matching.
      if ( (extraGroupingColumns_ == NULL || extraGroupingColumns_->entries() == 0) &&
            !getJbbSubset()->isIndirectGroupBy() )
      {
	// No extra grouping columns, so matching grouping list.
	matchingType_ = AMT_MAV_AQ_MG;
      }
      else
      {
	// Query needs a subset of the MV's grouping list.
	matchingType_ = AMT_MAV_AQ_DG;
      }
    }
    else
    {
      // MAV with join query JBB - not supposed to happen.
      assertLogAndThrow(CAT_MVCAND, LL_MVQR_FAIL, 
                        FALSE, QRLogicException, 
			"A MAV cannot match a join query.");
    }
  }
  else
  {
    // The MV is an MJV
    if (QueryHasGroupBy)
    {
      // Aggregate query with MJV.
      matchingType_ = AMT_MJV_AQ;
    }
    else
    {
      // Join query with MJV.
      matchingType_ = AMT_MJV_JQ;
    }
  }
}  // MVCandidate::initAggregateMatchingType()

/****************************************************************************/
/****************************************************************************/
NABoolean MVCandidate::matchPass1()
{
  if (getAggregateMatchingType() == AMT_MAV_AQ_DG &&
      mvDetails_->getJbbDetails()->hasHavingPredicates() )
  {
    // An MV with HAVING predicates cannot be used with rollup.
    logMVWasDisqualified("it has HAVING predicates, and the query needs Rollup.");
    return FALSE;
  }

  if (rangeMatching_.matchPass1() == FALSE)
    return FALSE;

  if (residualMatching_.matchPass1() == FALSE)
    return FALSE;

  if (groupingMatching_.matchPass1() == FALSE)
    return FALSE;

  if (outputMatching_.matchPass1() == FALSE)
    return FALSE;

  // Must be last Pass1 test before checking extrahub tables.
  if (joinPredMatching_.matchPass1() == FALSE)
    return FALSE;
  
  if (CheckExtraHubTables() == FALSE)
    return FALSE;

  if (lojMatching_.matchPass1() == FALSE)
    return FALSE;

  return TRUE;
}  // MVCandidate::matchPass1()

/****************************************************************************/
/****************************************************************************/
NABoolean MVCandidate::matchPass2()
{
  if (rangeMatching_.matchPass2() == FALSE)
    return FALSE;

  if (residualMatching_.matchPass2() == FALSE)
    return FALSE;

  if (groupingMatching_.matchPass2() == FALSE)
    return FALSE;

  if (outputMatching_.matchPass2() == FALSE)
    return FALSE;

  if (joinPredMatching_.matchPass2() == FALSE)
    return FALSE;

  if (lojMatching_.matchPass2() == FALSE)
    return FALSE;

  return TRUE;
}  // MVCandidate::matchPass1()

/****************************************************************************/
// Generate the Candidate element of the result descriptor including 
// the needed rewrite instructions.
/****************************************************************************/
QRCandidatePtr MVCandidate::generateDescriptor()
{
  // Start with the name of the MV.
  QRMVNamePtr mv = new(heap_) QRMVName(ADD_MEMCHECK_ARGS(heap_));
  mv->setMVName(mvDetails_->getMVName());
  mv->setTimestamp(mvDetails_->getRedefTimestampAsString());

  // Generate the QRCandidate object and populate it.
  QRCandidatePtr resultDesc = new(heap_) QRCandidate(ADD_MEMCHECK_ARGS(heap_));
  resultDesc->setMVName(mv);
  resultDesc->setIndirectGroupBy(getJbbSubset()->isIndirectGroupBy());

  NABoolean allIsWell = TRUE;
  // Now handle the GroupBy stuff.
  if(allIsWell)
    allIsWell = groupingMatching_.generateDescriptor(resultDesc);

  // Add range predicates.
  if(allIsWell)
    allIsWell = rangeMatching_.generateDescriptor(resultDesc);

  // Add range predicates.
  if(allIsWell)
    allIsWell = residualMatching_.generateDescriptor(resultDesc);

  // Add the output list.
  if(allIsWell)
    allIsWell = outputMatching_.generateDescriptor(resultDesc);

  // Add the back-join table list and join predicates.
  if(allIsWell)
    allIsWell = joinPredMatching_.generateDescriptor(resultDesc);

  // Add any NOT NULL predicates for inner over outer joins.
  if(allIsWell)
    allIsWell = lojMatching_.generateDescriptor(resultDesc);

  if (allIsWell)
  {
    QRLogger::log(CAT_MVCAND, LL_DEBUG,
      "Added MV %s to result descriptor.", mvDetails_->getMVName().data());

    return resultDesc;
  }
  else
  {
    deletePtr(resultDesc);
    disqualify();
    return NULL;
  }
}  // MVCandidate::generateDescriptor()

/*****************************************************************************/
/* Add new entries to the list of MV extra-hub or back-join tables 
/* used by the query.
/*****************************************************************************/
void MVCandidate::addExtraHubAndBackJoinTables(RewriteInstructionsItemPtr rewrite)
{
  extraHubTables_.insert(rewrite->getExtraHubTables());
  backJoinTables_.insert(rewrite->getBackJoinTables());
}

/*****************************************************************************/
// This MV candidate has been disqualified.
/****************************************************************************/
void MVCandidate::disqualify()
{
  jbbSubset_->getJbbCandidates()->addMatchInfo(disqualifiedReason_);
  disqualifiedReason_ = NULL;
  jbbSubset_->disqualify(this);
}

/****************************************************************************/
// Write to the log file why the MV was disqualified.
/****************************************************************************/
void MVCandidate::logMVWasDisqualified(const char* reason)
{
  // Avoid duplicate messages.
  if (wasDisqualified_)
    return;
  wasDisqualified_ = TRUE;

  char buffer[1024];
  const MVDetailsPtr mvDetails = getMvDetails();
  sprintf(buffer, "MV %s was disqualified because %s",
	  mvDetails->getMVName().data(), reason);
  QRLogger::log(CAT_MVCAND, LL_INFO, buffer);

  disqualifiedReason_ = new (heap_) NAString(buffer, heap_);
}

/****************************************************************************/
// Write to the log file why the MV was disqualified.
/****************************************************************************/
void MVCandidate::logMVWasDisqualified1(const char* reason, const NAString& arg1)
{
  char buffer[1024];
  sprintf(buffer, reason, arg1.data());
  logMVWasDisqualified(buffer);
}

/****************************************************************************/
// When an MV is disqualified while checking for optional information,
// mark it as qualified again.
/****************************************************************************/
void MVCandidate::reQualify()
{
  wasDisqualified_ = FALSE;
  delete disqualifiedReason_;
  disqualifiedReason_ = NULL;
}

/****************************************************************************/
// Use the query descriptor ID of a table to find its MV data.
/****************************************************************************/
const BaseTableDetailsPtr MVCandidate::getMvTableForQueryID(const NAString& id, NABoolean assertOnFailure)
{
  QRJoinSubGraphMapPtr queryMap = getQuerySubGraphMap();
  if (!queryMap->contains(id))
  {
    if (assertOnFailure)
    {
      assertLogAndThrow(CAT_MVCAND, LL_MVQR_FAIL,
                        FALSE, QRLogicException, 
			"Matching MV table not found in JoinGraphMap.");
    }
    return NULL;    
  }
  Int32 hashKeyIndex = queryMap->getIndexForTable(id);
  if (hashKeyIndex == -1)
  {
    assertLogAndThrow(CAT_MVCAND, LL_MVQR_FAIL,
                      assertOnFailure, QRLogicException, 
		      "Matching MV table not found in JoinGraphMap.");
    return NULL;
  }

  const JoinGraphTablePtr mvTable = getMvSubGraphMap()->getTableForIndex(hashKeyIndex);
  if (mvTable == NULL)
  {
    assertLogAndThrow(CAT_MVCAND, LL_MVQR_FAIL,
                      assertOnFailure, QRLogicException, 
		      "Matching MV table not found in JoinGraphMap.");
    return NULL;
  }

  return mvTable->getBaseTable();
}

/****************************************************************************/
// Use the MV descriptor ID of a table to find its Query data.
// Used only by WA
/****************************************************************************/
const BaseTableDetailsPtr MVCandidate::getQueryTableForMvID(const NAString& id, NABoolean assertOnFailure)
{
  QRJoinSubGraphMapPtr mvMap = getMvSubGraphMap();
  if (!mvMap->contains(id))
  {
    if (assertOnFailure)
    {
      assertLogAndThrow(CAT_MVCAND, LL_MVQR_FAIL,
                        FALSE, QRLogicException, 
			"Matching Query table not found in JoinGraphMap.");
    }
    return NULL;    
  }
  Int32 hashKeyIndex = mvMap->getIndexForTable(id);
  if (hashKeyIndex == -1)
  {
    assertLogAndThrow(CAT_MVCAND, LL_MVQR_FAIL,
                      assertOnFailure, QRLogicException, 
		      "Matching Query table not found in JoinGraphMap.");
    return NULL;
  }

  const JoinGraphTablePtr queryTable = getQuerySubGraphMap()->getTableForIndex(hashKeyIndex);
  if (queryTable == NULL)
  {
    assertLogAndThrow(CAT_MVCAND, LL_MVQR_FAIL,
                      assertOnFailure, QRLogicException, 
		      "Matching Query table not found in JoinGraphMap.");
    return NULL;
  }

  return queryTable->getBaseTable();
}

/****************************************************************************/
// This method is called between Pass 1 and Pass 2.
// Pass 1 finds MV extra-hub tables that are needed for output columns or
// predicates, and adds them to the extraHubTables_ list.
// Now, we need to verify that the extra-hub table is connected to the 
// rest of the sub-graph, using equi-join predicates that match the ones
// in the query. There is no need to match predicates because extra-hub 
// tables have no predicates on them by definition.
// The algorithm cannot handle self-joined extra-hub tables.
/****************************************************************************/
NABoolean MVCandidate::CheckExtraHubTables()
{
  if (extraHubTables_.entries() == 0)
    return TRUE;

  // Allocate a new subGraph and subGraphMap for the MV - the one we have is from MVMemo,
  // so its considered read-only.
  QRJoinSubGraphMapPtr mvSubGraphMap = new(heap_) 
    QRJoinSubGraphMap(*getMvSubGraphMap(), ADD_MEMCHECK_ARGS(heap_));

  // The query subGraph is shared between the MVCandidates that were found 
  // in the same MVMemo group, so work on a copy.
  QRJoinSubGraphMapPtr querySubGraphMap = new(heap_) 
    QRJoinSubGraphMap(*getJbbSubset()->getQuerySubGraphMap(), ADD_MEMCHECK_ARGS(heap_));

  // Loop over the list, each time picking an extra-hub table that is
  // directly connected to a table already in the matched sub-graph,
  // until all the tables are checked.
  do 
  {
    NABoolean extraHubTableWasAdded = FALSE;
    // Do the loop backwards because we remove checked extra-hub tables from the list.
    for (Int32 i=(CollIndex)extraHubTables_.entries()-1; i>=0; i--)
    {
      if (CheckAnExtraHubTable(extraHubTables_[i], 
			       mvSubGraphMap,    // IN/OUT
			       querySubGraphMap, // IN/OUT
			       extraHubTableWasAdded) == FALSE)
	 return FALSE;
    }  // for

    // Keep looping as long as there are extra-hub tables that are not connected, 
    // and as long as we keep adding them. 
    if (!extraHubTableWasAdded && extraHubTables_.entries() > 0)
    {
      logMVWasDisqualified1("Extra hub table %s is needed but cannot be matched.", 
  			    extraHubTables_[0]->data());
      return FALSE;
    }
//    assertLogAndThrow(extraHubTableWasAdded, QRLogicException, 
//		      "Extra-hub table is not connected to the hub.");

  } while (extraHubTables_.entries() > 0);

  // Update the MV new subGraphMap
  mvSubGraphMap_ = mvSubGraphMap;

  // We have added at least one extra-hub table - 
  // now we need to reorg the JBBSubsets accordingly.
  MVCandidatesForJBBPtr jbb = getJbbSubset()->getJbbCandidates();
  MVCandidatesForJBBSubsetPtr newJbbSubset = NULL;
  QRJoinSubGraphPtr querySubGraph = querySubGraphMap->getSubGraph();
  if (jbb->contains(querySubGraph))
  {
    // The JBBSubset with the new extra-hub tables already exists.
    newJbbSubset = jbb->getJbbSubsetFor(querySubGraph);
  }
  else
  {
    // Need to create a new JBBSubset.
    newJbbSubset = new (heap_) MVCandidatesForJBBSubset(jbb, ADD_MEMCHECK_ARGS(heap_));
    newJbbSubset->setSubGraphMap(querySubGraphMap);

    // Add the new JbbSubset to the JBB.
    jbb->insert(newJbbSubset);
  }

  // Move the candidate from the current JbbSubset to the new one.
  getJbbSubset()->remove(this);
  newJbbSubset->insert(this);
  setJbbSubset(newJbbSubset);

  return TRUE;
}  // CheckExtraHubTables()

//*****************************************************************************
//*****************************************************************************
NABoolean MVCandidate::CheckAnExtraHubTable(const NAString*      tableID, 
					    QRJoinSubGraphMapPtr mvSubGraphMap,
					    QRJoinSubGraphMapPtr querySubGraphMap,
					    NABoolean&		 extraHubTableWasAdded)
{
  QRJoinSubGraphPtr mvSubGraph    = mvSubGraphMap->getSubGraph();
  QRJoinSubGraphPtr querySubGraph = querySubGraphMap->getSubGraph();

  const QRTablePtr queryTableElement = 
    getQueryDetails()->getElementForID(*tableID)->downCastToQRTable();
  const JoinGraphTablePtr queryGraphTable = 
    querySubGraph->getParentGraph()->getTableByID(queryTableElement->getID());

  // If this extra-hub table is not directly connected to the rest of 
  // the subgraph, skip it for now.
  if (queryGraphTable->isConnectedTo(querySubGraph) == FALSE)
  {
    // Add additional needed extra-hub tables here...

    return TRUE;
  }

  // Find the corresponding MV table
  const QRTablePtr mvTableElement = getMvDetails()->getTableFromName(queryTableElement->getTableName());
  const JoinGraphTablePtr mvGraphTable = 
    mvSubGraph->getParentGraph()->getTableByID(mvTableElement->getID());

  if (matchPredsFromTableToSubGraph(mvSubGraph, 
				    querySubGraph, 
				    mvGraphTable, 
				    queryGraphTable) == FALSE)
  {
    // Match failed.
    // Release the sandbox subgraphs and disqualify the MVCandidate.
    deletePtr(mvSubGraphMap);
    deletePtr(querySubGraphMap);
    logMVWasDisqualified1("Extra hub table %s is needed but cannot be matched.", 
			  queryTableElement->getTableName().data());
    return FALSE;
  }

  // Add the table to both MV and query subgraphs.
  querySubGraph->addTable(queryGraphTable);
  mvSubGraph->addTable(mvGraphTable);

  // To make sure that Pass 2 matching and result desxcriptor generation will 
  // work correctly, add the tables to the SubGraphMaps (they will have the same index).
  CollIndex index = querySubGraph->entries();
  querySubGraphMap->addTable(index, queryGraphTable);
  mvSubGraphMap->addTable(index, mvGraphTable);

  // We are done with this table, remove it from the list.
  extraHubTables_.remove(tableID);

  extraHubTableWasAdded = TRUE;
  QRLogger::log(CAT_MVCAND, LL_DEBUG,
    "ExtraHub table %s was matched for MV %s.",
  	        queryTableElement->getTableName().data(),
  	        mvDetails_->getMVName().data());

  return TRUE;
}  // MVCandidate::CheckAnExtraHubTable()

//*****************************************************************************
//*****************************************************************************
NABoolean MVCandidate::isAUsedExtraHubTable(const NAString* tableID)
{
  CollIndex maxEntries = extraHubTables_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    if (*tableID == *(extraHubTables_[i]))
      return TRUE;
  }

  return FALSE;
}

//*****************************************************************************
// thisSubgraph and otherSubGraph are equivalent subgraphs (from the MV and
// query join graphs). This method checks if they will continue to match
// if an extra-hub table is added. It checks that ALL predicates connecting 
// this table to the subgraph match those of the other table/subgraph.
// The assumption is that this is the MV table/subgraph, while the other 
// is the query table/subgraph, so the query can have additional join
// predicates over those in the MV. Those extra predicates are returned as 
// rewrite instructions.
//*****************************************************************************
NABoolean MVCandidate::matchPredsFromTableToSubGraph(const QRJoinSubGraphPtr mvSubGraph,
						     const QRJoinSubGraphPtr querySubGraph, 
						     const JoinGraphTablePtr mvGraphTable,
						     const JoinGraphTablePtr queryGraphTable)
{
  // For each join pred on this table
  CollIndex maxEntries = mvGraphTable->getEqualitySets().entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    // Find the column its using
    const JoinGraphEqualitySetPtr mvEqSet = mvGraphTable->getEqualitySets()[i];
    const JoinGraphHalfPredicatePtr mvHalfPred = mvEqSet->findHalfPredTo(mvGraphTable);

    // Get the corresponding eqSet from the other join graph
    const JoinGraphEqualitySetPtr queryEqSet = queryGraphTable->getEqSetUsing(mvHalfPred);
    if (queryEqSet == NULL)
    {
      // Corresponding eqSet not found - match failed.
      return FALSE;
    }

    // Verify that the half-pred to the EQ set matches.
    const JoinGraphHalfPredicatePtr queryHalfPred = queryEqSet->findHalfPredTo(queryGraphTable);
    if (mvHalfPred->match(queryHalfPred) == FALSE)
      return FALSE;

    // Let the JoinPredMatching object continue with the matching from the EQ set to the subgraph.
    if (matchPredsFromEQSetToSubGraph(mvSubGraph, 
      				      querySubGraph, 
				      mvEqSet, 
				      queryEqSet,
				      mvHalfPred->getID()) == FALSE)
      return FALSE;
  }

  return TRUE;
}  // matchPredsFromTableToSubGraph()

// ***************************************************************************
// ***************************************************************************
NABoolean MVCandidate::matchPredsFromEQSetToSubGraph(const QRJoinSubGraphPtr	    mvSubGraph, 
						     const QRJoinSubGraphPtr	    querySubGraph, 
						     const JoinGraphEqualitySetPtr  mvEqSet,
						     const JoinGraphEqualitySetPtr  queryEqSet,
						     const NAString&		    mvHalfPredID) 
{
  NABoolean extraHubColumnChecked = FALSE;
  const HalfPredicateList& queryHalfPreds = queryEqSet->getHalfPredicates();
  CollIndex maxEntries = queryHalfPreds.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    const JoinGraphHalfPredicatePtr queryHalfPred = queryHalfPreds[i];
    const JoinGraphTablePtr	    queryTable = queryHalfPred->getTable();
    if (querySubGraph->contains(queryTable))
    {
      // We found a join pred that's connected to the subgraph - match it.
      // Find the corresponding table in the MV join graph. 
      // ??? Don't assert if a match is not found, because maybe the join pred
      // ??? does not exist in the MV.
      const BaseTableDetailsPtr mvBaseTable = getMvTableForQueryID(queryTable->getID(), FALSE);
      if (mvBaseTable == NULL)
      {
	// Do we ever get here???
	assertLogAndThrow(CAT_MVCAND, LL_MVQR_FAIL,
                          FALSE, QRLogicException, 
			  "MV hub table corresponding to query hub not found.");
      }
      else
      {
	// We have the MV hub table corresponding to the query hub table.
	// Now find its halfPred.
	const NAString& mvID = mvBaseTable->getID();
	const JoinGraphTablePtr mvTable = mvSubGraph->getParentGraph()->getTableByID(mvID);
	const JoinGraphHalfPredicatePtr mvHalfPred = mvEqSet->findHalfPredTo(mvTable);

	if (mvHalfPred == NULL)
	{
	  // This query pred has no corresponding mv pred.
	  // Handle rewrite instructions here (Not implemented yet).
	  //if (addJoinPredForExtraHubTable(mvHalfPredID, queryHalfPred) == FALSE)
	  return FALSE;
	}
	else
	{
	  // Finally we have both the MV halfPred and its corresponding query halfPred.
	  // Match them.
	  if (mvHalfPred->match(queryHalfPred) == FALSE)
	    return FALSE;
	}

	// Continue matching with the next halfPred.
	continue;
      }

    }
  }

  return TRUE;
}  // MVCandidate::matchPredsFromEQSetToSubGraph()


const NAString MVCandidate::trueString_  = "TRUE";
const NAString MVCandidate::falseString_ = "FALSE";

/****************************************************************************/
/****************************************************************************/
void MVCandidate::cacheTableID(const NAString* tableID, 
                               NABoolean       isOutside, 
                               const NAString* extraHubID)
{
  if (extraHubID != NULL)
  {
    tableIDCache_.insert(tableID, extraHubID);
  }
  else if (isOutside)
  {
    tableIDCache_.insert(tableID, &trueString_);
  }
  else 
  {
    tableIDCache_.insert(tableID, &falseString_);
  }
}

/****************************************************************************/
/****************************************************************************/
NABoolean MVCandidate::probeCacheForTableID(const NAString*  tableID, 
                                            NABoolean&       isOutside, 
                                            const NAString*& extraHubID)
{
  const NAString* result = tableIDCache_.getFirstValue(tableID);
  if (result == NULL)
    return FALSE;
  else if (*result == trueString_)
  {
    isOutside = TRUE;
  }
  else if (*result == falseString_)
  {
    isOutside = FALSE;
  }
  else
  {
    isOutside = FALSE;
    extraHubID = result;
  }

  return TRUE;
}

/****************************************************************************/
/****************************************************************************/
JBBDetailsPtr MVCandidate::getQueryJbbDetails()
{
  if (queryJbbDetails_)
    return queryJbbDetails_;

  const NAString& id = queryJbb_->getID();

  JBBDetailsPtrList& jbbs = queryDetails_->getJbbDetailsList();
  CollIndex maxEntries = jbbs.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    JBBDetailsPtr jbbDetails = jbbs[i];
    if (jbbDetails->getJbbDescriptor()->getID() == id)
    {
      queryJbbDetails_ = jbbDetails;
      return jbbDetails;
    }
  }

  return NULL;
}

/****************************************************************************/
/****************************************************************************/
void MVCandidate::addOutputColumn(RewriteInstructionsItemPtr rewrite, NABoolean makeACopy)
{
  RewriteInstructionsItemPtr newRewrite = rewrite;

  if (makeACopy)
  {
    // Make a copy to avoid deleting it twice.
    newRewrite = new (heap_) RewriteInstructionsItem(*rewrite, ADD_MEMCHECK_ARGS(heap_));
  }

  outputMatching_.addRewriteInstructions(newRewrite);
}

//========================================================================
//  Class MVCandidatesForJBBSubset
//========================================================================

/****************************************************************************/
/****************************************************************************/
MVCandidatesForJBBSubset::~MVCandidatesForJBBSubset()
{
  // The subGraph was not deleted with the hubIterator, because we set the
  // stillUsed flag, so delete it now.
  deletePtr(querySubGraphMap_);

  // Delete all the MVCndidates.
  for (CollIndex i=0; i<candidateList_.entries(); i++)
    deletePtr(candidateList_[i]);
}

/****************************************************************************/
/****************************************************************************/
NABoolean MVCandidatesForJBBSubset::operator==(const MVCandidatesForJBBSubset& other) const
{
  return getSubGraph() == other.getSubGraph();
}

/****************************************************************************/
/****************************************************************************/
const NAString* MVCandidatesForJBBSubset::getKeyObject() const
{
  return getSubGraph()->getFastKey();
}

/****************************************************************************/
/****************************************************************************/
void MVCandidatesForJBBSubset::setSubGraphMap(QRJoinSubGraphMapPtr querySubGraphMap) 
{ 
  querySubGraphMap_ = querySubGraphMap; 

  // Mark that this subGraph is being used for rewrite instructions, so that
  // it will not be deleted when the HubIterator is deleted.
  querySubGraphMap->getSubGraph()->setStillUsed(TRUE);
}

/****************************************************************************/
/****************************************************************************/
QRJoinSubGraphPtr MVCandidatesForJBBSubset::getSubGraph() const
{
  return querySubGraphMap_->getSubGraph();
}

/****************************************************************************/
/* Parse the MV_AGE cqd and translate it to seconds.
/* A 32 bit number should be enough for calculating seconds.
/****************************************************************************/
Int32 parseMVAge(const NAString& mvAge)
{
  Int32 result = 0;
  float number=0;
  char textChars[20];

  if (mvAge.length() < 15)
  {
    if (sscanf(mvAge.data(), "%f %s", &number, textChars) == 2)
    {
      const NAString text(textChars);
      if (!text.compareTo("Seconds", NAString::ignoreCase))
      {
	result = (Int32)floor(number);
      }
      else if (!text.compareTo("Minutes", NAString::ignoreCase))
      {
	result = (Int32)floor(number*60);
      }
      else if (!text.compareTo("Hours", NAString::ignoreCase))
      {
	result = (Int32)floor(number*60*60);
      }
      else if (!text.compareTo("Days", NAString::ignoreCase))
      {
	result = (Int32)floor(number*60*60*24);
      }
    }
  }

  if (result == 0)
  {
    QRLogger::log(CAT_MVCAND, LL_ERROR,
      "Invalid setting for MV_AGE default value: %s, Using only fresh MVs.",
      mvAge.data());

  }

  return result;
}

/****************************************************************************/
// Create an MVCandidate object and insert it into the JBBSubset.
/****************************************************************************/
void MVCandidatesForJBBSubset::insert(MVDetailsPtr	    mv, 
				      QRJBBPtr		    queryJbb,
				      NABoolean		    isPreferredMatch, 
				      GroupingList*	    extraGroupingColumns,
				      QRJoinSubGraphMapPtr  mvMap,
				      CollHeap*		    heap)
{
  DescriptorDetailsPtr queryDetails =  jbbCandidates_->getAllCandidates()->getQueryDetails();
  NAString excuse(heap);
  if (mv->isInitialized() == FALSE)
  {
    // Disqualify uninitialized MVs
    excuse = " was skipped because it is not initialized";
  }
  else
  {
    QRDescriptorPtr desc = queryDetails->getDescriptor();
    QRQueryDescriptorPtr queryDesc = static_cast<QRQueryDescriptorPtr>(desc);
    QRQueryMiscPtr misc = queryDesc->getMisc();
    const NAString& mvAge = misc->getMVAge();
    
    switch (misc->getRewriteLevel())
    {
      case MRL_FRESH:
	// Allow only ON STATEMENT MVs.
	if (mv->isImmediate() == FALSE)
	{
	  // Disqualify stale MVs.
	  excuse = " was skipped because it is not completey fresh";
	}
	break;
	      
      case MRL_STALE:
	if (mv->isImmediate())
	{
	  // Allow immediate MVs. No need to calculate MV_AGE.
	  break;
	}

	if ( mv->isUMV() )
	{
	  // Disqualify UMVs.
	  excuse = " was skipped because it is user maintained";
	  break;
	}

	// Allow all MVs that were refreshed at least MV_AGE  ago
	if ( mvAge != "")
	{
	  const Int32 mvAgeSeconds = parseMVAge(mvAge);
          const Int64 mvRefreshTS = mv->getRefreshTimestamp()/1000000;
	  const Int64 nowTS = NA_JulianTimestamp()/1000000;
	  Int32 refreshAge = (Int32)(nowTS - mvRefreshTS);
          QRLogger::log(CAT_MVCAND, LL_DEBUG,
            "MV_AGE is: %d seconds, RefreshAge is: %d seconds.", mvAgeSeconds, refreshAge);
	  if (mvAgeSeconds < refreshAge)
	  {
	    // Disqualify stale MVs.
	    excuse = " was skipped because it is stale.";
	  }
	}
	break;
	      
      case MRL_OLD:
	if ( mv->isUMV() )
	{
	  // Disqualify UMVs.
	  excuse = " was skipped because it is user maintained";
	}
	break;

      case MRL_UMVS:
        // All MVs are allowed here.
        break;
	      
      case MRL_OFF:
        // MVQR is OFF. Not supposed to get a descriptor in this case.
      default:
        assertLogAndThrow1(CAT_MVCAND, LL_ERROR, 
  	                   FALSE, QRLogicException, 
	                   "Invalid value for MVQR_REWRITE_LEVEL: %d", (Int32)misc->getRewriteLevel());
    }
  }
  
  // Create a new MVCandidate object for it.
  MVCandidatePtr newCandidate = new(heap) 
    MVCandidate(mv, queryDetails, queryJbb, this, ADD_MEMCHECK_ARGS(heap));

  if (mvMap != NULL)
    newCandidate->setMvSubGraphMap(mvMap);

  newCandidate->init(isPreferredMatch, extraGroupingColumns);

  insert(newCandidate);
  
  if (excuse != "")
  {
    // The reason we insert the MVCandidate and then disqualify it is that
    // we want the disqualification message to be added to the result descriptor.
    NAString msg(heap);
    msg = "MV ";
    msg.append(mv->getMVName());
    msg.append(excuse);
    newCandidate->logMVWasDisqualified(msg);
    newCandidate->disqualify();
  }
}  // MVCandidatesForJBBSubset::insert()

/****************************************************************************/
// Insert an MV candidate object into this JBBSubset.
// Keep the list ordered by MV name.
/****************************************************************************/
void MVCandidatesForJBBSubset::insert(MVCandidatePtr mv) 
{ 
  const NAString& newMVName = mv->getMvDetails()->getMVName();
  CollIndex maxEntries = candidateList_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    if (candidateList_[i]->getMvDetails()->getMVName() > newMVName)
    {
      candidateList_.insertAt(i, mv); 
      return;
    }
  }

  candidateList_.insertAt(maxEntries, mv); 
}

/****************************************************************************/
// Generate the JBBSubset element of the result descriptor including the 
// rewrite instructions and all the MV candidates inside it.
/****************************************************************************/
QRJbbSubsetPtr MVCandidatesForJBBSubset::generateDescriptor(CollHeap* heap)
{
  // No candidates - no work.
  if (candidateList_.entries() == 0)
    return NULL;

  // Construct the new QRJbbSubset object.
  QRJbbSubsetPtr resultDesc = new(heap) QRJbbSubset(ADD_MEMCHECK_ARGS(heap));
  resultDesc->setGroupBy(hasGroupBy_);
  if (hasGroupBy_)
  {
    // Add a reference to the query NodeID of the GroupBy node.
    const NAString& groupbyID = 
      getJbbCandidates()->getQueryJbb()->getGroupBy()->getID();
    resultDesc->setRef(groupbyID);
  }

  // For each table included in the subgraph
  QRJoinSubGraphPtr   subGraph = querySubGraphMap_->getSubGraph();
  for(subGraph->reset(); subGraph->hasNext(); subGraph->advance())
  {
    JoinGraphTablePtr table = subGraph->getCurrent();

    // Add it to the JBBSubset descriptor.
    QRTablePtr tableDesc = new(heap) QRTable(heap);
    tableDesc->setTableName(table->getName());
    tableDesc->setRef(table->getID());
    resultDesc->addTable(tableDesc);
  }

  // If candidates are disqualified during the loop itself,
  // Don't remove them from the list.
  isLoopingOnCandidates_ = TRUE;

  // Now add the MVCandidates.
  CollIndex maxEntries = candidateList_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    MVCandidatePtr candidate = candidateList_[i];
    if (candidate == NULL)
    {
      // The candidate was disqualified.
      continue;
    }

    QRCandidatePtr candidateXml;
    try
      {
        candidateXml = candidate->generateDescriptor();
      }
    catch (...)
      {
        candidate->logMVWasDisqualified("an exception was thrown.");
        candidate->disqualify();  // Propagate the Info element to the result descriptor.

        candidateXml = NULL;
      }

    if (candidateXml == NULL)
    {
      // The candidate was disqualified when generating the descriptor.
      continue;
    }
    resultDesc->addCandidate(candidateXml);
  }

  if (resultDesc->getCandidateList()->entries() == 0)
  {
    // None of the candidates survived the result descriptor generation.
    deletePtr(resultDesc);
    return NULL;
  }

  return resultDesc;
}  // MVCandidatesForJBBSubset::generateDescriptor()

/*****************************************************************************/
// Remove the MVCandidate from the list, because it has been disqualified.
// Skip the removal if we are on the final loop of generating the result descriptor
// because that will throw the loop off its index.
/****************************************************************************/
void MVCandidatesForJBBSubset::disqualify(MVCandidatePtr candidate)
{
  if (!isLoopingOnCandidates_)
  {
     candidateList_.remove(candidate);
  }
}

/*****************************************************************************/
// Add this MVCandidate into the master list of all the MVCandiates for this query.
/****************************************************************************/
void MVCandidatesForJBBSubset::registerCandidate(MVCandidatePtr candidate)
{
  jbbCandidates_->registerCandidate(candidate);
}

//========================================================================
//  Class MVCandidatesForJBB
//========================================================================

/****************************************************************************/
/****************************************************************************/
MVCandidatesForJBB::MVCandidatesForJBB(QRJBBPtr			queryJbb, 
				       MVCandidateCollectionPtr	allCandidates,
				       ADD_MEMCHECK_ARGS_DEF(CollHeap* heap))
  : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
    ,jbbSubsets_(heap)
    ,queryJbb_(queryJbb)
    ,allCandidates_(allCandidates)
    ,infoList_(heap)
{ 
}

/****************************************************************************/
/****************************************************************************/
MVCandidatesForJBB::~MVCandidatesForJBB()
{
  CollIndex maxEntries = jbbSubsets_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    deletePtr(jbbSubsets_[i]);
  }

  jbbSubsets_.clear();
}

/****************************************************************************/
/****************************************************************************/
void MVCandidatesForJBB::insert(MVCandidatesForJBBSubsetPtr jbbSubset) 
{ 
  CollIndex pos = 0;
  while (pos < jbbSubsets_.entries() && jbbSubsets_[pos]->getKeyObject() )
    pos++;

  jbbSubsets_.insertAt(pos, jbbSubset); 
}


/****************************************************************************/
/****************************************************************************/
NABoolean MVCandidatesForJBB::contains(QRJoinSubGraphPtr subGraph) const
{
  CollIndex pos = 0;
  while (pos < jbbSubsets_.entries() && jbbSubsets_[pos]->getKeyObject() )
    pos++;

  return (pos < jbbSubsets_.entries() && 
          jbbSubsets_[pos]->getKeyObject() == subGraph->getFastKey() );
}

/****************************************************************************/
/****************************************************************************/
MVCandidatesForJBBSubsetPtr MVCandidatesForJBB::getJbbSubsetFor(QRJoinSubGraphPtr subGraph) const
{
  CollIndex pos = 0;
  while (pos < jbbSubsets_.entries() && jbbSubsets_[pos]->getKeyObject() )
    pos++;

  if (pos < jbbSubsets_.entries() && 
      jbbSubsets_[pos]->getKeyObject() == subGraph->getFastKey() )
    return jbbSubsets_[pos];
  else
    return NULL;
}

/****************************************************************************/
/****************************************************************************/
void MVCandidatesForJBB::addMatchInfo(const NAString* info)
{
  // Keep info lines sorted.
  CollIndex maxEntries = infoList_.entries();
  if (maxEntries == 0)
  {
    infoList_.insert(info);
  }
  else
  {
    for (CollIndex i=0; i<maxEntries; i++)
    {
      if (*infoList_[i] > *info)
      {
        infoList_.insertAt(i, info);
        return;
      }
    }

    infoList_.insertAt(maxEntries, info);
  }
}

/****************************************************************************/
// Generate the result descriptor for this JBB.
/****************************************************************************/
QRJbbResultPtr MVCandidatesForJBB::generateDescriptor(CollHeap* heap)
{
  // Allocate the JBBResult element.
  QRJbbResultPtr resultDesc = new(heap) QRJbbResult(ADD_MEMCHECK_ARGS(heap));
  // Set the reference to the JBB in the query descriptor.
  resultDesc->setRef(queryJbb_->getID());

  CollIndex maxEntries = jbbSubsets_.entries();
//  for (CollIndex i=0; i<maxEntries; i++)
  for (Int32 i=maxEntries-1; i>=0; i--)
  {
    MVCandidatesForJBBSubsetPtr jbbSubset = jbbSubsets_[i];
    QRJbbSubsetPtr jbbSubsetResult = jbbSubset->generateDescriptor(heap);
    if (jbbSubsetResult != NULL)
      resultDesc->addJbbSubset(jbbSubsetResult);
  }

  maxEntries = infoList_.entries();
  for (CollIndex j=0; j<maxEntries; j++)
  {
    QRInfoPtr info = new (heap) QRInfo(ADD_MEMCHECK_ARGS(heap));
    info->setText(*(infoList_[j]));
    resultDesc->addInfoItem(info);
  }

  // If all the MV candidates of all the JBBSubsets for this JBB has been 
  // disqualified, Don't denerate an element for it.
  if (resultDesc->getJbbSubsets().entries() == 0 && maxEntries == 0)
  {
    deletePtr(resultDesc);
    resultDesc = NULL;
  }

  return resultDesc;
}  // MVCandidatesForJBB::generateDescriptor()

/*****************************************************************************/
// Add this MVCandidate into the master list of all the MVCandiates for this query.
/****************************************************************************/
void MVCandidatesForJBB::registerCandidate(MVCandidatePtr candidate)
{
  allCandidates_->registerCandidate(candidate);
}


//========================================================================
//  Class MVCandidateCollection
//========================================================================

MVCandidateCollection::~MVCandidateCollection()
{
  allCandidates_.clear();

  CollIndex maxEntries = jbbs_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
  {
    MVCandidatesForJBBPtr jbb = jbbs_[i];
    deletePtr(jbb);
  }

  deletePtr(queryDetails_);
}

/****************************************************************************/
// Generate the result descriptor for this query.
/****************************************************************************/
QRResultDescriptorPtr MVCandidateCollection::generateEmptyResultDescriptor(CollHeap* heap)
{
  // Allocate the result descriptor.
  QRResultDescriptorPtr resultDesc = new(heap) QRResultDescriptor(ADD_MEMCHECK_ARGS(heap));

  // Init the version element.
  NAString versionString(CURRENT_VERSION, heap);  
  QRVersionPtr descVersion = new (heap) QRVersion(ADD_MEMCHECK_ARGS(heap));
  descVersion->setVersionString(versionString);
  resultDesc->setVersion(descVersion);

  return resultDesc;
}  //  MVCandidateCollection::generateEmptyResultDescriptor()

/****************************************************************************/
// Generate the result descriptor for this query.
/****************************************************************************/
QRResultDescriptorPtr MVCandidateCollection::generateResultDescriptor(CollHeap* heap)
{
  // Allocate the result descriptor.
  QRResultDescriptorPtr resultDesc = generateEmptyResultDescriptor(heap);

  // Add all the JBB elements.
  for (CollIndex i=0; i<jbbs_.entries(); i++)
  {
    QRJbbResultPtr jbbResult = jbbs_[i]->generateDescriptor(heap);
    if (jbbResult != NULL)
      resultDesc->addJbbResult(jbbResult);
  }

  return resultDesc;
}  //  MVCandidateCollection::generateResultDescriptor()

/****************************************************************************/
// Add this MVCandidate into the master list of all the MVCandiates for this query.
/*****************************************************************************/
void MVCandidateCollection::registerCandidate(MVCandidatePtr candidate)
{
  allCandidates_.insert(candidate);
}

/*****************************************************************************/
// Matching failed - remove the MV candidate from all the lists, and
// rerlease its memory.
/*****************************************************************************/
void MVCandidateCollection::disqualifyCandidate(MVCandidatePtr candidate, Int32 index)
{
  candidate->disqualify();	  // Remove from the JBBSubset
  allCandidates_.removeAt(index); // Remove from the master list
  deletePtr(candidate);		  // Delete it.
}


/****************************************************************************/
// Run the matching algorithms on all the MVCandidate objects.
// This is a temporary method, until the MV candidate reduction algorithm 
// is implemented in a separate method.
/*****************************************************************************/
void MVCandidateCollection::doMatching()
{
  // For now, just go over all the MV candidates, and run the matching 
  // algorithms one by one.
  // The loop is running backwards so we can delete MV candidates that are
  // disqualified, without breaking the loop index.

  // Start with running Pass 1 tests on all candidates.
  NABoolean matched = FALSE;
  for (Int32 i=allCandidates_.entries()-1; i>=0; i--)
  {
    MVCandidatePtr candidate = allCandidates_[i];
    if (!candidate->wasDisqualified())
    {
      // Run the output list matching, Pass 1.
      try
        {
          matched = candidate->matchPass1();
        }
      catch (...)
        {
          // Skip only this candidate on exception, and proceed with matching
          // for the rest.
          QRLogger::log(CAT_MVCAND, LL_MVQR_FAIL,
            "Disqualifying candidate %s due to exception on pass 1",
                      candidate->getMvDetails()->getMVName().data());
          matched = FALSE;
        }
  
      if (!matched)
      {
        // Matching failed - disqualify the MV candidate.
        disqualifyCandidate(candidate, i);
        continue;
      }
    }
  }

  // Now do Pass 2 tests on the remaining candidates.
  for (Int32 i=allCandidates_.entries()-1; i>=0; i--)
  {
    MVCandidatePtr candidate = allCandidates_[i];

    // Run the output list matching, Pass 2.
    try
      {
        matched = candidate->matchPass2();
      }
    catch (...)
      {
        // Skip only this candidate on exception, and proceed with matching
        // for the rest.
        QRLogger::log(CAT_MVCAND, LL_MVQR_FAIL,
          "Disqualifying candidate %s due to exception on pass 2",
                    candidate->getMvDetails()->getMVName().data());
        matched = FALSE;
      }

    if (!matched)
    {
      // Matching failed - disqualify the MV candidate.
      disqualifyCandidate(candidate, i);
      continue;
    }
  }
}

//========================================================================
//  Class MVPair
//========================================================================

/*****************************************************************************/
/*****************************************************************************/
MVPair::MVPair(const QRJoinSubGraphMapPtr    firstMV, 
               const QRJoinSubGraphMapPtr    otherMV,
	       ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
  : MVCandidate(otherMV->getMVDetails(), 
                firstMV->getMVDetails(), 
                firstMV->getMVDetails()->getJbbDetails()->getJbbDescriptor(), 
                NULL,
	        ADD_MEMCHECK_ARGS_PASS(heap))
   ,querySubGraphMap_(firstMV)
   ,mvSubGraphMap_(otherMV)
   ,rangePredList_(heap)
   ,residualPredList_(heap)
{
   QRRangePredListPtr rangePredList = 
     getMvDetails()->getJbbDetails()->getJbbDescriptor()->getHub()->getRangePredList();
   if (rangePredList)
     rangePredList_.insert(rangePredList->getList());

   QRResidualPredListPtr residualPredList =
     getMvDetails()->getJbbDetails()->getJbbDescriptor()->getHub()->getResidualPredList();
   if (residualPredList)
     residualPredList_.insert(residualPredList->getList());
}

/*****************************************************************************/
/*****************************************************************************/
NABoolean MVPair::checkRangePredicate(QRRangePredPtr querySidePred)
{
  QRRangePredPtr mvSidePred = getRangeMatching().checkPredicate(querySidePred);
  if (mvSidePred)
  {
    rangePredList_.remove(mvSidePred);
    return TRUE;
  }
  else
    return FALSE;
}

/*****************************************************************************/
/*****************************************************************************/
NABoolean MVPair::checkResidualPredicate(QRExprPtr querySidePred)
{
  QRExprPtr mvSidePred = getResidualMatching().checkPredicate(querySidePred);
  if (mvSidePred)
  {
    residualPredList_.remove(mvSidePred);
    return TRUE;
  }
  else
    return FALSE;
}

/*****************************************************************************/
/*****************************************************************************/
void MVPair::intersectColumnBitmapsForTable(const NAString& queryTableID, 
                                            XMLBitmap&      rangeBitmap, 
                                            XMLBitmap&      residualBitmap)
{
  // Find the MV table ID using the MVMemo mapping.
  BaseTableDetailsPtr mvTableDetails = getMvTableForQueryID(queryTableID);

  // Get the MV predicate bitmaps
  const XMLBitmap& mvRangeBits = mvTableDetails->getRangeBits();
  rangeBitmap.intersectSet(mvRangeBits);
  const XMLBitmap& mvResidualBits = mvTableDetails->getResidualBits();
  residualBitmap.intersectSet(mvResidualBits);
}

/*****************************************************************************/
/*****************************************************************************/
NABoolean MVPair::checkForLOJ(QRTablePtr queryTable)
{
  // Find the MV table ID using the MVMemo mapping.
  BaseTableDetailsPtr mvTableDetails = getMvTableForQueryID(queryTable->getID());
  return mvTableDetails->getTableElement()->hasLOJParent();
}

