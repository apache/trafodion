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
* File:         MVJoinGraph.cpp
* Description:  Definition of classes for verifying the join graph is
*               fully connected, and for finding a good ordering of tables.
*
* Created:      7 March 2000
* Language:     C++
* Status:       $State: Exp $
*
*
******************************************************************************
*/

#include "Sqlcomp.h"
#include "AllItemExpr.h"
#include "AllRelExpr.h"
#include "GroupAttr.h"
#include "MVInfo.h"
#include "MVJoinGraph.h"

//============================================================================
//===========================  class MVJoinTable =============================
//============================================================================

NABoolean MVJoinTable::operator==(const MVJoinTable &other) const
{
  if(&other == this)
    return TRUE;
  else
    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
// Get all the ref constraints from this NATable, filter the ones that are to
// tables in this graph, and mark them on both tables. For example, use the
// RI on Orderes and Customers: O->C ( O is referencing C ). Because of the 
// semantics of the order of the tables in the join graph, in order for RI
// optimization to be utilized, C must appear in the graph solution AFTER O.
// The result is that C has an incoming bit for table O, and O has an 
// outgoing bit for table C.
// Other conditions that must be met for the RI to be usable:
// 1. C must have non-empty, insert only delta.
// 2. O must not be inner tables of left joins.
// 3. Each of the columns of the RI constraint must be covered by a predicate.
//    So if O(a,b) is referencing C(x,y) the join should use these two equal 
//    predicates: (O.a = C.x) AND (O.b = C.y).
// In this method, this table is O, and otherTable is C.
Lng32 MVJoinTable::markRiConstraints(BindWA *bindWA, MVInfo *mvInfo)
{
  LIST (MVUsedObjectInfo*)& usedObjects = mvInfo->getUsedObjectsList();
  if (usedObjects[tableIndex_]->isInnerTableOfLeftJoin())
    return 0;	// O must not be inner table of a left join.

  Lng32 howManyRIs=0;
  const AbstractRIConstraintList& refConstraints = 
    naTable_->getRefConstraints();

  for (CollIndex i=0; i<refConstraints.entries(); i++)
  {
    RefConstraint *const ref = (RefConstraint *const)(refConstraints[i]);

    CMPASSERT(ref->getOperatorType() == ITM_REF_CONSTRAINT);
    // Ignore self referencing RIs.
    if (ref->selfRef())
      continue;

    // Find the table the RI is referencing.
    const NAString& otherTableName = 
      ref->getOtherTableName().getQualifiedNameAsString();
    MVJoinTable *otherTable =
      mvInfo->getJoinGraph()->getTableObjectFor(&otherTableName);
    if (otherTable == NULL)
      continue;	 // The other table must be on the graph.

    if (otherTable->deltaType_ != INSERTONLY_DELTA)
      continue;  // C must be insert only.

    Lng32 otherTableIndex = otherTable->getTableIndex();

    // The RI must be covered by equal predicates on the same columns
    LIST(Lng32) myCols(NULL);
    LIST(Lng32) otherCols(NULL);
    ref->getMyKeyColumns(myCols);
    ref->getOtherTableKeyColumns(bindWA, otherCols);
    CMPASSERT(myCols.entries() == otherCols.entries());

    NABoolean matchingPredicatesExist=TRUE;
    for (CollIndex currentCol=0; currentCol<myCols.entries(); currentCol++)
    {
      if (!mvInfo->isEqPredicateBetween(naTable_->getTableName(),
					myCols[currentCol],
					ref->getOtherTableName(),
					otherCols[currentCol]))
	matchingPredicatesExist = FALSE;
    }
    if (!matchingPredicatesExist)
      continue;

    // OK - we found a qualifying RI that we can use for optimization.
    // Now mark the bits.
    markRiTo(otherTableIndex);
    otherTable->markRiFrom(getTableIndex());
    howManyRIs++;
  }
  return howManyRIs;
}	

//////////////////////////////////////////////////////////////////////////////
// Is there a predicate from any table in the connected set to this table?
NABoolean MVJoinTable::isOnPredicate(const MVJoinGraphState& state) const
{
  // The route bitmap is a bitmap of the connected set.
  // Mask only those tables of the route we have a predicate to.
  NABitVector coveredPredicates(predicateBitmap_);

  coveredPredicates.intersectSet(state.getRouteBitmap());
  // One covered predicate is all we need.
  return coveredPredicates.entries() > 0;
}

//////////////////////////////////////////////////////////////////////////////
// Is there an RI from any table is the connected set to this table?
NABoolean MVJoinTable::isOnRI(const MVJoinGraphState& state) const
{
  // If there are no RIs at all, skip the check.
  if (state.getNofRIs() == 0)
    return FALSE;

  // The route bitmap is a bitmap of the connected set.
  // Mask only those tables of the route we have an incoming RI from.
  NABitVector coveredIncomingRis(incomingRiBitmap_);

  coveredIncomingRis.intersectSet(state.getRouteBitmap());
  // One covered RI is all we need.
  return coveredIncomingRis.entries() > 0;
}

//////////////////////////////////////////////////////////////////////////////
// How good is this table as a candidate for the next table on the route?
// Score categories :
// 1. RI from the connected set to this table
//    This RI can be utilized now.
// 2. outgoing RIs from this table to tables in the available set.
//    These RIs can be utilized later.
// 3. incoming RIs from the available set to this table (negative score).
//    Taking this table next will not allow us to utilize the RI later.
// 4. Predicates from this table to tables in the available set.
//    Better chances for connectivity.
// This method is called O(n^3) times.
Lng32 MVJoinTable::calcHeuristics(const MVJoinGraphState& state) const
{
  // These are the weights used by the heuristic function.
  enum {  WEIGHT_ON_RI       =  100,
	  WEIGHT_OUTGOING_RI =  100,
	  WEIGHT_PREDICATE   =    1,
	  WEIGHT_INCOMING_RI = -200 };

  Lng32 score = 0;

  // Check the predicates first.
  NABitVector predicatesToAS(predicateBitmap_);
  predicatesToAS.intersectSet(state.getAvailableBitmap());

  Lng32 preds = (Lng32) predicatesToAS.entries();
  score += preds*WEIGHT_PREDICATE;       // Category 4

  // If there are no RIs, the result is based on predicates alone.
  if (state.getNofRIs() == 0)
    return score;

  // Check outgoing RIs to the available set.
  NABitVector outRisToAS(outgoingRiBitmap_);
  outRisToAS.intersectSet(state.getAvailableBitmap());

  Lng32 outRIs = (Lng32) outRisToAS.entries();
  score += outRIs*WEIGHT_OUTGOING_RI;   // Category 2

  // Check incoming RIs from the available set.
  NABitVector inRisFromAS(incomingRiBitmap_);
  inRisFromAS.intersectSet(state.getAvailableBitmap());

  Lng32 inRIs  = (Lng32) inRisFromAS.entries();
  score += inRIs*WEIGHT_INCOMING_RI;    // Category 3

  // Is this table on RI now?
  if (isOnRI(state))
    score += WEIGHT_ON_RI;		// Category 1

  return score;
}

//////////////////////////////////////////////////////////////////////////////
#ifndef NDEBUG
static void BitmapToString(const NABitVector& bm, NAString& text)
{
  CollIndex lastBit;

  bm.lastUsed(lastBit);
  for (Int32 i=lastBit; i>=0; i--)
  {
    if (bm.testBit(i))
      text += "1  ";
    else
      text += "0  ";
  }
}

void MVJoinTable::print(FILE* ofd, const char* indent, const char* title)const
{
  char buffer[20];
  const char *deltaTypeText = NULL;
  switch (deltaType_)
  {
    case EMPTY_DELTA     : deltaTypeText = "Empty Delta";      break;
    case INSERTONLY_DELTA: deltaTypeText = "InsertOnly Delta"; break;
    case NONEMPTY_DELTA  : deltaTypeText = "Nonempty Delta";   break;
  }
  fprintf(ofd, "Table no. %d: %s (%s),\n", 
	  tableIndex_, tableName_.data(), deltaTypeText);

  NAString titleString(" ");
  CollIndex lastBit;
  
  predicateBitmap_.lastUsed(lastBit);
  for (Int32 i=lastBit; i>=0  ; i--)
  {
	snprintf( buffer, 20, "%d", i );
    titleString += buffer;
    titleString += ", ";
  }
  NAString predicateString(" ");
  NAString inRiString(" ");
  NAString outRiString(" ");
  BitmapToString(predicateBitmap_,  predicateString);
  BitmapToString(incomingRiBitmap_, inRiString);
  BitmapToString(outgoingRiBitmap_, outRiString);

  fprintf(ofd, "\tTableIndices : %s.\n", titleString.data());
  fprintf(ofd, "\tPredicates to: %s.\n", predicateString.data());
  fprintf(ofd, "\tIncoming RIs : %s.\n", inRiString.data());
  fprintf(ofd, "\tOutgoing RIs : %s.\n", outRiString.data());
}

void MVJoinTable::display() const 
{ 
  print(); 
}
#endif

//============================================================================
//===========================  class MVJoinGraphSolution =======================
//============================================================================

//////////////////////////////////////////////////////////////////////////////
// Ctor. Initialize the route_ array.
MVJoinGraphSolution::MVJoinGraphSolution(Lng32 nofTables, CollHeap *heap)
  : route_(heap, nofTables), 
    entries_(0),
    bitmap_(heap),
    riTables_(heap, nofTables)
{
  for (Lng32 i=0; i<nofTables; i++)
    route_.insert(i, -1);
}

//////////////////////////////////////////////////////////////////////////////
// Add a table to the route_
void MVJoinGraphSolution::pushTable(Lng32 tableIndex, NABoolean isOnRi)
{ 
  route_[entries_] = tableIndex; 
  bitmap_.setBit(tableIndex);
  entries_++;
  if (isOnRi)
    riTables_.insert(tableIndex);
}

//////////////////////////////////////////////////////////////////////////////
// Start fresh with a new clean route.
void MVJoinGraphSolution::reset()
{
  for (Lng32 i=0; i<entries_; i++)
  {
    bitmap_.resetBit(route_[i]);
    route_[i] = -1;
  }

  entries_ = 0;
  riTables_.clear();
}

//////////////////////////////////////////////////////////////////////////////
#ifndef NDEBUG
void MVJoinGraphSolution::print(FILE* ofd, const char* indent, const char* title)const
{
  fprintf(ofd, "\nMVJoinGraphSolution (Score: %d): ", getScore());
  for (Lng32 i=0; i<entries_; i++)
    fprintf(ofd, "%d, ", route_[i]);
  fprintf(ofd, ".\n");
}

void MVJoinGraphSolution::display() const 
{ 
  print(); 
}
#endif

//============================================================================
//===========================  class MVJoinGraphState ========================
//============================================================================

//////////////////////////////////////////////////////////////////////////////
// Ctor. Initialize the availableBitmap according to the available set.
MVJoinGraphState::MVJoinGraphState(Lng32              nofTables, 
				   Lng32              nofRIs, 
				   const MVTableSet& reorderGroup, 
				   CollHeap         *heap)
  : nofTables_(nofTables),
    nofRI_(nofRIs),
    connectedSet_(heap, nofTables),
    availableSet_(reorderGroup, heap),
    availableBitmap_(heap),
    currentRoute_(nofTables, heap),
    bestRoute_(nofTables, heap)
{
  for (CollIndex i=0; i<reorderGroup.entries(); i++)
    availableBitmap_.setBit(reorderGroup[i]);
}

//////////////////////////////////////////////////////////////////////////////
// Pick the next table to be added to the route, according to the
// heuristic function.
// If this is the first table of the route, pick it from the notStartedSet_,
// otherwise, pick it from the availableSet.
// Return -1 if there are no predicates from any table in the connected set
// to any table in the available set. This means the graph is not connected.
// This method does not yet take into account left outer joins.
Lng32 MVJoinGraphState::pickNextTable(const MVJoinGraph& joinGraph, 
				     NABoolean          isFirst) const
{
  Lng32 maxResult = -1000000; // Negative scores are bad but legal.
  Lng32 maxTable  = -1;

  const MVTableSet& availableTables =
    isFirst ? joinGraph.getNotStartedSet() : availableSet_;

  // Find the score for each of the available tables.
  for (CollIndex i=0; i<availableTables.entries(); i++)
  {
    Lng32 currentTableIndex = availableTables[i];
    const MVJoinTable *currentTable = 
      joinGraph.getTableObjectAt(currentTableIndex);

    // If this is the first table on the route, there is nothing in the
    // connected set to have a predicate to. Otherwise, a table is 
    // considered only if it has a predicate to the connected set.
    if (!isFirst && !currentTable->isOnPredicate(*this))
      continue;

    // Calculate the heuristic score for the current table.
    Lng32 currentResult = currentTable->calcHeuristics(*this);

    // Is it the best so far?
    if (currentResult > maxResult)
    {
      maxResult = currentResult;
      maxTable  = currentTableIndex;
    }
  }

  return maxTable;
}

//////////////////////////////////////////////////////////////////////////////
// Update the state according to the next table that was chosen.
void MVJoinGraphState::nextTableIs(Lng32 tableIndex, NABoolean isOnRI)
{
  // Add the table to the connected set.
  connectedSet_.insert(tableIndex);

  // Remove it from the available set.
  availableSet_.remove(tableIndex);
  availableBitmap_.resetBit(tableIndex);

  // Add it to the route.
  currentRoute_.pushTable(tableIndex, isOnRI);
}

//////////////////////////////////////////////////////////////////////////////
// Start fresh looking for a new solution.
// Reset all data members except bestRoute_.
void MVJoinGraphState::reset(const MVTableSet& reorderGroup)
{
  availableSet_ = reorderGroup;
  connectedSet_.clear();
  currentRoute_.reset();

  for (CollIndex i=0; i<reorderGroup.entries(); i++)
    availableBitmap_.setBit(reorderGroup[i]);
}

//////////////////////////////////////////////////////////////////////////////
// Is the current solution better than bestRoute_? 
// If so - may the better route win.
void MVJoinGraphState::chooseBestSolution()
{
  if (bestRoute_.isEmpty()     || 
      (currentRoute_.getScore() > bestRoute_.getScore()))
    bestRoute_ = currentRoute_;
} 

//============================================================================
//===========================  class MVJoinGraph =============================
//============================================================================

//////////////////////////////////////////////////////////////////////////////
// Find the table name in the hash table, get the table index and return
// the table object. This method is used primarily during the initialization
// of predicates and RIs.
MVJoinTable *MVJoinGraph::getTableObjectFor(const NAString *tableName)
{
  return tableHash_.getFirstValue(tableName);
}

//////////////////////////////////////////////////////////////////////////////
// Add a prepared table to the graph.
void MVJoinGraph::addTable(MVJoinTable *newNode)
{
  usedTables_[newNode->getTableIndex()] = newNode;
  tableHash_.insert(newNode->getTableName(), newNode);
}	

//////////////////////////////////////////////////////////////////////////////
// Prepare a table object and add it to the graph.
void MVJoinGraph::addTable(Lng32           tableIndex, 
			   Lng32           usedObjectsIndex, 
			   const NATable *naTable)
{
  MVJoinTable *newNode = new(heap_) 
    MVJoinTable(tableIndex, usedObjectsIndex, nofTables_, naTable, heap_);
  addTable(newNode);
}	

//////////////////////////////////////////////////////////////////////////////
// Given a table qualified name, returns the table's name as an Ansi string
NAString MVJoinGraph::fixName(const QualifiedName& tableName) const
{
  // If the name already has double-quotes mark, that means it is already
  // in an Ansi format. if we return it ...AsAnsiString, another set of
  // double-quotes will be added, and we don't want to do that.
  if(tableName.getObjectName().contains('"'))
  {
    return tableName.getQualifiedNameAsString();
  }
  return tableName.getQualifiedNameAsAnsiString();
}

//////////////////////////////////////////////////////////////////////////////
// Mark an equal predicate between two tables.
void MVJoinGraph::markPredicateBetween(const QualifiedName& leftTable, 
				       const QualifiedName& rightTable)
{
  NAString leftTableName(fixName(leftTable));
  NAString rightTableName(fixName(rightTable));

  // Find the index of the left table.
  MVJoinTable *leftTableObj  = tableHash_.getFirstValue(&leftTableName);
  Lng32   leftIndex          = leftTableObj->getTableIndex();

  // Find the index of the right table.
  MVJoinTable *rightTableObj = tableHash_.getFirstValue(&rightTableName);
  Lng32   rightIndex         = rightTableObj->getTableIndex();

  // Mark the predicate on both tables.
  leftTableObj->markPredicateTo(rightIndex);
  rightTableObj->markPredicateTo(leftIndex);
}	

//////////////////////////////////////////////////////////////////////////////
// Mark the RI constraints for all the tables.
void MVJoinGraph::markRiConstraints(BindWA *bindWA, MVInfo *mvInfo)
{
  for (Lng32 i=0; i<nofTables_; i++)
    nofRI_ += usedTables_[i]->markRiConstraints(bindWA, mvInfo);
}	

//////////////////////////////////////////////////////////////////////////////
// The first table was chosen, now find a solution from there.
// Return TRUE if a solution was found, FALSE if not.
NABoolean MVJoinGraph::findSolutionFrom(MVJoinGraphState& state)
{
  while (!state.isComplete())
  {
    Lng32 nextTable = state.pickNextTable(*this, FALSE);
    if (nextTable == -1)
    {
      // No predicates to tables in the available set - the graph is not
      // fully connected.
      return FALSE;
    }

    // Can we utilize an RI for optimization on this table?
    NABoolean isOnRI = usedTables_[nextTable]->isOnRI(state);
    // Update the graph state with the new table.
    state.nextTableIs(nextTable, isOnRI);
  } 

  // Compare the solution we just found to the best solution so far.
  state.chooseBestSolution();

  return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
// This is the DDL version of the algorithm:
//   - RIs are not used at all.
//   - Only one attempt is made to reach a solution.
//   - We only want to know if the graph is fully connected or not.
//   - In order to support left outer joins, this is still missing:
//     * Instead of one group of all tables, the tables should be divided
//       to reorder groups according to the location of the left join, where
//       an inner table of a left join is a group by itself.
//     * The algorithm should be run for each group separatly.
//     * If a group is not connected, tables in the group that are not
//       mentioned in the join predicates of the left join above them, can be
//       moved to the group above the inner table. Then run it again.
NABoolean MVJoinGraph::isFullyConnected()
{
  if (nofTables_==1)
  {
    // This is a single table MV - of course its connected.
    return TRUE;
  }

#ifndef NDEBUG
  if ( CmpCommon::getDefault(MV_DUMP_DEBUG_INFO) == DF_ON )
      display();
#endif

  // start with one group of all tables.
  for (Lng32 i=0; i<nofTables_; i++)
    reorderGroupSet_.insert(i);
  notStartedSet_ = reorderGroupSet_;
  MVJoinGraphState state(nofTables_, nofRI_, reorderGroupSet_, heap_);

  // Choose a first table to start from (according to predicates).
  Lng32 nextTable = state.pickNextTable(*this, TRUE);
  CMPASSERT(nextTable != -1);
  state.nextTableIs(nextTable, FALSE);

  // Find a solution from this table.
  NABoolean isConnected = findSolutionFrom(state);

  if (isConnected)
  {
    // The state object will be gone when we exit this method, so save
    // the solution for later (to order the tables).
    theSolution_ = state.getCurrentRoute();

#ifndef NDEBUG
  if ( CmpCommon::getDefault(MV_DUMP_DEBUG_INFO) == DF_ON )
    theSolution_.display();
#endif
  }
  return isConnected;
}

//////////////////////////////////////////////////////////////////////////////
// Mark the connected order of the tables in the MVInfo used objects list.
// (used in DDL only).
void MVJoinGraph::markOrderOfUsedObjects(MVInfoForDDL *mvInfo)
{
  const GraphRoute&	    finalRoute  = theSolution_.getRoute();
  LIST (MVUsedObjectInfo*)& usedObjects = mvInfo->getUsedObjectsList();

  if (nofTables_ == 1)
  {
    // This is a single table MV, so the algorithm was not run.
    // Just mark the table as table no. 0.
    MVUsedObjectInfo *usedInfo = usedObjects[0];
    usedInfo->setOrdinalNumber(0);
    return;
  }

  for (Lng32 i=0; i<nofTables_; i++)
  {
    // Get the tableIndex
    Lng32 tableIndex = finalRoute[i];
    // Find the index into the usedObjects list.
    Lng32 usedObjectsIndex = usedTables_[tableIndex]->getUsedObjectsIndex();
    // Get the usedObject from the list.
    MVUsedObjectInfo *usedInfo = usedObjects[usedObjectsIndex];

    usedInfo->setOrdinalNumber(i);
  }
}

//////////////////////////////////////////////////////////////////////////////
// This is the MDL version of the algorithm, used during a multi-delta refresh.
//   - RIs are used to determine the BEST order of the tables.
//   - We compute a solution from each and every table in the graph, and use
//     the number of usable RIs on the solution to determine which one wins.
void MVJoinGraph::findBestOrder()
{
  CMPASSERT(nofTables_>1);

#ifndef NDEBUG
    display();
#endif

  // start with one group of all tables.
  for (Lng32 i=0; i<nofTables_; i++)
    reorderGroupSet_.insert(i);
  notStartedSet_ = reorderGroupSet_;
  MVJoinGraphState state(nofTables_, nofRI_, reorderGroupSet_, heap_);

  NABoolean done = FALSE;
  while (!done && notStartedSet_.entries()>0)
  {
    // Choose a table to start from
    Lng32 firstTable = state.pickNextTable(*this, TRUE);
    CMPASSERT(firstTable != -1);
    state.nextTableIs(firstTable, FALSE);
    notStartedSet_.remove(firstTable);

    // Find a solution from this table.
    if (findSolutionFrom(state))
    {
      state.chooseBestSolution();
      if (state.getBestRoute().getScore() == nofRI_)
	done = TRUE;  // This is as good as it gets.
    }
    else
    {
      // If we get here the graph is not fully connected!
      CMPASSERT(FALSE);
    }

    if (!done)
      state.reset(reorderGroupSet_);
  } 

  CMPASSERT(!state.getBestRoute().isEmpty());

  // The state object will be gone when we exit this method, so save
  // the solution for later.
  theSolution_ = state.getBestRoute();

#ifndef NDEBUG
  theSolution_.display();
#endif
}

//////////////////////////////////////////////////////////////////////////////
// Return the route of the solution, translated into indices to the used
// objects array.
void MVJoinGraph::getRouteOfSolution(GraphRoute& translatedRoute) const
{
  for (Lng32 i=0; i<nofTables_; i++)
  {
    Lng32 usedObjectIndex = usedTables_[i]->getUsedObjectsIndex();
    translatedRoute.insertAt(i, usedObjectIndex);
  }
}

//////////////////////////////////////////////////////////////////////////////
// Return TRUE if all the deltas are either empty or insert only.
NABoolean MVJoinGraph::isInsertOnlyRefresh() const
{
  for (Lng32 i=0; i<nofTables_; i++)
  {
    if (usedTables_[i]->isNonEmptyDelta())
      return FALSE;
  }

  return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
#ifndef NDEBUG
void MVJoinGraph::print(FILE* ofd, const char* indent, const char* title)const
{
  fprintf(ofd, "\nMVJoinGraph:\n");
  for (Lng32 i=0; i<nofTables_; i++)
    usedTables_.at(i)->print(ofd); 
}

void MVJoinGraph::display() const 
{ 
  print(); 
}
#endif
