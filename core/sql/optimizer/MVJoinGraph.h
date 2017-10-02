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
#ifndef MVJOINGRAPH_H
#define MVJOINGRAPH_H

/* -*-C++-*-
******************************************************************************
*
* File:         MVJoinGraph.h
* Description:  Definition of classes for verifying the join graph is
*               fully connected, and for finding a good ordering of tables,
*				according to the join predicates and RI constraints.
*
* Created:      7 March 2000
* Language:     C++
* Status:       $State: Exp $
*
*
******************************************************************************
*/


//============================================================================
//=== The classes in this module implement the join graph algorithm.
//=== This algorithm is used in two different situations:
//=== 1. During DDL (CREATE MV) in oredr to verify that the join graph is
//===    fully connected, and give the tables an initial order.
//=== 2. During DML (multi-delta refresh) to find the optimal ordering of
//===    tables, taking into account current RI constraints, in order to 
//===    optimizer multi-delta refresh.
//=== In order to find the optimal solution in reasonable time, the
//=== algorithm uses a heuristic function. This function can be tuned if
//=== we find a testcase where it gives a sub-optimal solution. See details
//=== in MVJoinTable::calcHeuristics().
//============================================================================

#include <ComSmallDefs.h>
#include <Collections.h>

// Classes defined in this module
class MVJoinTable;
class MVJoinGraphState;
class MVJoinGraphSolution;
class MVJoinGraph;

// Forward definitions
class NATable;

// This hash table is used to speed up finding the MVJoinTable object from
// the table name. It's used primarily when initializing the predicate
// and RI bitmaps.
typedef NAHashDictionary<const NAString, MVJoinTable> MVJoinTableHash;

// A set of tableIndex values, each an index to MVJoinGraph::usedTables_.
typedef SET(Lng32) MVTableSet;

typedef ARRAY(Lng32) GraphRoute;

//////////////////////////////////////////////////////////////////////////////
// This class holds the information of the connections a specific table has
// to the other tables on the join graph.
// This class implements the rules and heuristic functions of the algorithm.
class MVJoinTable : public NABasicObject
{
public:
  MVJoinTable(Lng32 tableIndex, 
	      Lng32 usedIndex, 
	      Lng32 nofTables, 
	      const NATable *naTable,
	      CollHeap *heap)
  :  tableIndex_(tableIndex),
     usedObjectsIndex_(usedIndex),
     tableName_(naTable->getTableName().getQualifiedNameAsAnsiString(),
                heap),
     naTable_(naTable),
     deltaType_(EMPTY_DELTA),
     predicateBitmap_ (heap),
     incomingRiBitmap_(heap),
     outgoingRiBitmap_(heap)
  {}
  
  // operator== required for use in templates
  NABoolean operator==(const MVJoinTable &other) const;

  // Accessors
  Lng32             getTableIndex()       const { return tableIndex_; }
  Lng32             getUsedObjectsIndex() const { return usedObjectsIndex_; }
  const NAString  *getTableName()	 const { return &tableName_; }
  const NABitVector&  getPredicateBitmap()  const { return predicateBitmap_; }
  const NABitVector&  getIncomingRiBitmap() const { return incomingRiBitmap_; }
  const NABitVector&  getOutgoingRiBitmap() const { return outgoingRiBitmap_; }
  NABoolean        isEmptyDelta()        const { return deltaType_ == EMPTY_DELTA; }
  NABoolean        isInsertOnlyDelta()   const { return deltaType_ == INSERTONLY_DELTA; }
  NABoolean        isNonEmptyDelta()     const { return deltaType_ == NONEMPTY_DELTA; }

  // Mutators
  void setNonEmptyDelta()	   { deltaType_ = NONEMPTY_DELTA; }
  void setInsertOnlyDelta()	   { deltaType_ = INSERTONLY_DELTA; }
  void markPredicateTo(Lng32 table) { predicateBitmap_.setBit(table); }
  void markRiTo(Lng32 table) 	   { outgoingRiBitmap_.setBit(table); }
  void markRiFrom(Lng32 table) 	   { incomingRiBitmap_.setBit(table); }
  Lng32 markRiConstraints(BindWA *bindWA, MVInfo *mvInfo);

  // Is there a predicate from the Connected Set to me?
  NABoolean isOnPredicate(const MVJoinGraphState& state) const;
  // Is there an RI from the Connected Set to me?
  NABoolean isOnRI(const MVJoinGraphState& state) const;
  // How good am I as a candidate for the next table on the route?
  Lng32 calcHeuristics(const MVJoinGraphState& state) const;

#ifndef NDEBUG
  void display() const;
  void print(FILE* ofd = stdout,
  const char* indent = DEFAULT_INDENT,
  const char* title = "MVJoinTable") const;
#endif

private:
  enum DeltaType { EMPTY_DELTA, INSERTONLY_DELTA, NONEMPTY_DELTA };

  const Lng32	  tableIndex_;		// Index into MVJoinGraph::usedTables_
  const Lng32	  usedObjectsIndex_;	// Index into MVInfo::usedObjectsList_
  const NAString  tableName_;		// For persistance of the name pointer 
  const NATable  *naTable_;		// For finding the RIs
  NABitVector     predicateBitmap_;     // Who do I have a prediucate to?
  NABitVector     incomingRiBitmap_;    // Tables with RIs referencing me?
  NABitVector     outgoingRiBitmap_;    // Tables with RIs referenced by me?
  DeltaType	  deltaType_;		// Is delta Empty/Readonly/Nonempty
}; // MVJoinTable

//////////////////////////////////////////////////////////////////////////////
// This class describes an ordered traverse of the join tree, where each
// table is connected to at lease one of its predecessors.
class MVJoinGraphSolution : public NABasicObject
{
public:
  MVJoinGraphSolution(Lng32 nofTables, CollHeap *heap);

  MVJoinGraphSolution(const MVJoinGraphSolution& other, CollHeap *heap)
  : route_(other.route_, heap), 
    entries_(other.entries_),
    bitmap_(bitmap_),
    riTables_(other.riTables_, heap)
  {}

  NABoolean           isEmpty()     const { return entries_ == 0; }
  Lng32		      getScore()    const { return riTables_.entries(); }
  const GraphRoute&   getRoute()    const { return route_; }
  const NABitVector&  getBitmap()   const { return bitmap_; }
  const MVTableSet&   getRiTables() const { return riTables_; }
  NABoolean isTableOnRI(Lng32 tableIndex) const 
    { return riTables_.contains(tableIndex); }

  // Add a table to the route.
  void pushTable(Lng32 tableIndex, NABoolean isOnRi);

  void reset();

#ifndef NDEBUG
  void display() const;
  void print(FILE* ofd = stdout,
	     const char* indent = DEFAULT_INDENT,
	     const char* title = "MVJoinGraph") const;
#endif

private:
  GraphRoute	route_;	    // The list of tableIndex values.
  Lng32		entries_;   // The length of the current route.
  NABitVector   bitmap_;    // Bitmap of route tables.
  MVTableSet	riTables_;  // Set of tables for which RI opt. can be used.
}; // MVJoinGraphSolution

//////////////////////////////////////////////////////////////////////////////
// Holds the state of the graph during the traverse.
// The main data members are:
// 1. The Connected Set	- the tables already traversed on a connected route.
// 2. The Available Set - the tables not yet included in the connected set.
// 3. The Current Route - Holds the order of the tables in the solution being
//    constructed, and the score (RIs that are utilized for optimization).
// 4. Results List      - A list of results, from which the best one will be
//    chosen (used only during Refresh activation).
class MVJoinGraphState : public NABasicObject
{
public:
  MVJoinGraphState(Lng32 nofTables, 
		   Lng32 nofRIs, 
		   const MVTableSet& reorderGroup, 
		   CollHeap *heap);

  // Accessors
  Lng32  getNofRIs()			 const { return nofRI_; }
  const MVTableSet& getConnectedSet()	 const { return connectedSet_; }
  const MVTableSet& getAvailableSet()	 const { return availableSet_; }
  const NABitVector& getAvailableBitmap() const { return availableBitmap_; }
  const NABitVector& getRouteBitmap()    const { return currentRoute_.getBitmap(); }
  NABoolean isComplete()		 const { return (availableSet_.entries() == 0); }
  const MVJoinGraphSolution& getBestRoute()    { return bestRoute_; }
  const MVJoinGraphSolution& getCurrentRoute() const { return currentRoute_; }

  // Pick the next table to be added to the route, according to the
  // heuristic function.
  Lng32 pickNextTable(const MVJoinGraph& joinGraph, NABoolean isFirst) const;

  // Update the state according to the next table that was chosen.
  void nextTableIs(Lng32 tableIndex, NABoolean isOnRI);

  // Start fresh for a new reorder group.
  void  reset(const MVTableSet& reorderGroup);

  // Is the current solution better than bestRoute_? 
  void chooseBestSolution();

private:
  const Lng32	nofTables_;             // The number of tables in the join.
  const Lng32	nofRI_;                 // The total number of RIs
  MVTableSet	connectedSet_;          // The connected set.
  MVTableSet	availableSet_;          // The available set.
  NABitVector   availableBitmap_;       // A bitmap representing the available set.
  MVJoinGraphSolution  currentRoute_;   // The current route being explored.
  MVJoinGraphSolution  bestRoute_;      // The best route so far.
}; // MVJoinGraphState

//////////////////////////////////////////////////////////////////////////////
// This is the abstraction of the join graph algorithm.
// This algorithm is used in two different times for different purposes:
// DDL: During CREATE MV, this algorithm verifies that the join graph
//      is fully connected, based on the join predicates.
// DML: During multi-delta INTERNAL REFRESH, predicates and RI constraints 
//      are used to determine the best ordering of the tables, so that as many 
//      RI optimizations as possible can be utilized.
//
// Handling of left joins is not yet supported.
// In order to support left joins, the tables should be divided into reorder
// groups, according to the location of the left joins. If a group is not
// fully connected, tables from this group can be pushed up above the left 
// join to another group if they are not referenced by predicates in the
// left join.
class MVJoinGraph : public NABasicObject
{
public:
  MVJoinGraph(Lng32 nofTables, CollHeap *heap)
  : nofTables_(nofTables),
    nofRI_(0),
    usedTables_(heap, nofTables),
    reorderGroupSet_(heap, nofTables),
    notStartedSet_(heap, nofTables),
    tableHash_(hashKey, nofTables*2, FALSE, heap), // Pass NAString::hashKey as function ptr
    theSolution_(nofTables, heap),
    heap_(heap)
  {
    for (Lng32 i=0; i<nofTables; i++)
      usedTables_.insert(i, NULL);
  }

  // Initializing the data structures.
  void addTable(Lng32 tableIndex, Lng32 usedObjectsIndex, const NATable *naTable);
  void addTable(MVJoinTable *newNode);
  void markPredicateBetween(const QualifiedName& leftTable, 
			    const QualifiedName& rightTable);
  void markRiConstraints(BindWA *bindWA, MVInfo *mvInfo);
  void setNofTables(Lng32 finalNumber) { nofTables_ = finalNumber; }

  // Accessors
  MVJoinTable *getTableObjectFor(const NAString *tableName);
  MVJoinTable *getTableObjectAt(Lng32 tableIndex) const 
    { return usedTables_[tableIndex]; }
  const MVTableSet& getNotStartedSet() const
    { return notStartedSet_; }

  // Methods used during the DDL activation only:

  // Run the DDL algorithm.
  NABoolean isFullyConnected();
  //  Mark the order of the tables in the MVInfo used objects list.
  void markOrderOfUsedObjects(MVInfoForDDL *mvInfo);

  // Methods used during the DML activation only:

  // Run the DML algorithm.
  void  findBestOrder();
  // The first table was chosen, now find a solution from there.
  NABoolean findSolutionFrom(MVJoinGraphState& state);
  const MVJoinGraphSolution& getSolution() const { return theSolution_; }
  void  getRouteOfSolution(GraphRoute&  translatedRoute) const;
  NABoolean isInsertOnlyRefresh() const;

#ifndef NDEBUG
  void display() const;
  void print(FILE* ofd = stdout,
	     const char* indent = DEFAULT_INDENT,
	     const char* title = "MVJoinGraph") const;
#endif

private:
  // helpers
  NAString fixName(const QualifiedName& tableName) const;

private:
  Lng32			nofTables_;       // The number of tables in the join.
  Lng32			nofRI_;           // The total number of RIs
  ARRAY(MVJoinTable *)  usedTables_;      // accessing tableIndex objects by
                                          // index (tableIndex).
  MVTableSet		reorderGroupSet_; // A subset of usedTables. Prepare
                                          // for running the algorithm on a subset
                                          // of the join graph.
  MVTableSet		notStartedSet_;   // Tables we haven't started a route from.
  MVJoinTableHash	tableHash_;       // Accessing MVJoinTable objects by
                                          // the table name.
  MVJoinGraphSolution	theSolution_;     // The solution found.
  CollHeap 	       *heap_;
}; // MVJoinGraph

#endif // MVJOINGRAPH_H
