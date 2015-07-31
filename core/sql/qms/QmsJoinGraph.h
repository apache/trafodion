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
// File:         QRJoinGraph.h
// Description:  
//               
//               
//               
//
// Created:      12/11/07
// ***********************************************************************

// _MEMSHAREDPTR;_MEMCHECK



#include "NABasicObject.h"
#include "NAString.h"
#include "Collections.h"
#include "Int64.h"
#include "QRSharedPtr.h"
#include "QRDescriptor.h"

class JoinGraphTable;
class JoinGraphHalfPredicate;
class JoinGraphEqualitySet;
class QRJoinGraph;
class QRJoinSubGraph;
class QRJoinSubGraphMap;

#ifdef _MEMSHAREDPTR
typedef QRIntrusiveSharedPtr<JoinGraphTable>		JoinGraphTablePtr;
typedef QRIntrusiveSharedPtr<JoinGraphHalfPredicate>	JoinGraphHalfPredicatePtr;
typedef QRIntrusiveSharedPtr<JoinGraphEqualitySet>	JoinGraphEqualitySetPtr;
typedef QRIntrusiveSharedPtr<QRJoinGraph>		QRJoinGraphPtr;
typedef QRIntrusiveSharedPtr<QRJoinSubGraph>		QRJoinSubGraphPtr;
typedef QRIntrusiveSharedPtr<QRJoinSubGraphMap>		QRJoinSubGraphMapPtr;
#else
typedef JoinGraphTable*					JoinGraphTablePtr;
typedef JoinGraphHalfPredicate*				JoinGraphHalfPredicatePtr;
typedef JoinGraphEqualitySet*				JoinGraphEqualitySetPtr;
typedef QRJoinGraph*					QRJoinGraphPtr;
typedef QRJoinSubGraph*					QRJoinSubGraphPtr;
typedef QRJoinSubGraphMap*				QRJoinSubGraphMapPtr;
#endif

typedef NAPtrList<JoinGraphHalfPredicatePtr>		HalfPredicateList;
typedef NAPtrList<JoinGraphEqualitySetPtr>		EqualitySetList;
typedef NAPtrList<JoinGraphTablePtr>		        JoinGraphTableList;
typedef NAPtrArray<JoinGraphTablePtr>			JoinGraphTableArray;
typedef NASubArray<JoinGraphTablePtr>			JoinGraphTableSubArray;
typedef NAString*                                       NAStringPtr;
typedef NAPtrList<QRJoinSubGraphPtr>			SubGraphList;
typedef NAPtrList<QRJoinSubGraphMapPtr>			SubGraphMapList;

typedef SharedPtrValueHash<const NAString, JoinGraphTable>	        JoinGraphTableHash;
typedef SharedPtrValueHashIterator<const NAString, JoinGraphTable>      JoinGraphTableHashIterator;
typedef SharedPtrValueHash<const NAString, QRJoinSubGraph>		SubGraphHash;
typedef SharedPtrValueHashIterator<const NAString, QRJoinSubGraph>	SubGraphHashIterator;

#ifndef _QRJOINGRAPH_H_
#define _QRJOINGRAPH_H_

enum OperationType { OP_INSERT = 0, OP_SEARCH, OP_INSERT_TOP };

#include "QmsMVDetails.h"
#include "QmsMVCandidate.h"
#include "QmsSelfJoinHandler.h"


/**
 * A JoinGraphTable is a node in the join graph.
 * It holds the table name, its descriptor ID, and pointers to predicates.
 * The ordinal number of a table is its index into the QRJoinGraph table array.
 * The temp number is used when building the MVMemo hash key.
 *
 * @see QRJoinGraph
 * @see JoinGraphHalfPredicate
 * @see JoinGraphEqualitySet
 * @see QRJoinSubGraph
 *****************************************************************************
*/
class JoinGraphTable : public NAIntrusiveSharedPtrObject
{
public:

  /**
   * Public constructor.
   * @param num		  The ordinal number - the index of this table into the QRJoinGraph table array.
   * @param tableElement  Table element from the descriptor.
   * @param heap	  Heap from which to allocate internal data members.
   */
  JoinGraphTable(CollIndex	  num, 
		 const QRTablePtr tableElement,
		 NABoolean	  isHub,
		 const BaseTableDetailsPtr baseTable,
                 ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
     ,ordinalNumber_(num)
     ,tempNumber_(-1)
     ,keyColumns_(0)
     ,reducedConnections_(0)
     ,tableElement_(tableElement)
     ,isHub_(isHub)
     ,baseTable_(baseTable)
     ,predList_(heap)
     ,name_(tableElement->getTableName())
     ,isSelfJoinTable_(FALSE)
  {}

  virtual ~JoinGraphTable() {}

  NABoolean operator==(const JoinGraphTable& other)
  {
    return ordinalNumber_ == other.ordinalNumber_;
  }

  /** Get the ordinal number - the index into the QRJoinGraph table array. */
  CollIndex getOrdinalNumber() const { return ordinalNumber_; }
  /** Get the temp number - used when building the MVMemo hash key.  */
  Int32 getTempNumber() const { return tempNumber_; }
  /** Get the table name. */
  const NAString& getName() const { return tableElement_->getTableName(); }
  /** Get the table element ID from the MV/query descriptor. */
  const NAString& getID() const { return tableElement_->getID(); }  
  /** Get the table element from the descriptor. */
  const QRTablePtr getTableElement() const { return tableElement_; }

  const BaseTableDetailsPtr getBaseTable() const { return baseTable_; }

  NABoolean isHub() const { return isHub_; };

  /** Get the list of equality sets involving this table. */
  const EqualitySetList& getEqualitySets() { return predList_; }

  /** Set the table's ordinal number. Used after the QRJoinGraph table array is re-ordered. */
  void setOrdinalNumber(CollIndex ix)  { ordinalNumber_ = ix; }
  /** Set the table's temp number. Used when marking the table during generation of a sub-graph.*/
  void setTempNumber(Int32 ix)  { tempNumber_ = ix; }

  /** Add an equality set. */
  void addEqualitySet(JoinGraphEqualitySetPtr pred)
  {
    predList_.insert(pred);
  }

  /** Remove an equality set. */
  void removeEqualitySet(JoinGraphEqualitySetPtr pred)
  {
    predList_.remove(pred);
  }

  /** Clear the table's equality sets list. */
  void clearEqualitySets() { predList_.clear(); }

  /** Add a line with this table's information to the graphical join graph in the output parameter string. */
  void dumpGraph(NAString& to);

  /** Is this table directly connected to the sub-graph? */
  NABoolean isConnectedTo(const QRJoinSubGraphPtr subGraph) const;

  /** Find an equality set from the ones this table is connected to, that match otherHalfPred, which is from a different join graph. */
  JoinGraphEqualitySetPtr getEqSetUsing(JoinGraphHalfPredicatePtr otherHalfPred);

  /** Check if predicates on this table are on its key columns. */
  NABoolean checkPredsOnKey(CollHeap* heap);

  void reduceConnection()
  {
    reducedConnections_++;
  }

  NABoolean isOnTheEdge();

  void setSelfJoinTable()
  {
    isSelfJoinTable_ = TRUE;
  }

  NABoolean isSelfJoinTable()
  {
    return isSelfJoinTable_;
  }

  void getAndReducePredicateColumnsPointingToMe(ElementPtrList& pointingCols);

  Int32 getDegree();

private:
  // Copy construction/assignment not defined.
  JoinGraphTable(const JoinGraphTable&);
  JoinGraphTable& operator=(const JoinGraphTable&);

private:
  /** Index into the QRJoinGraph table array. */
  CollIndex	    ordinalNumber_;
  /** Used when building the MVMemo hash key. */
  Int32		    tempNumber_;
  /** Used for indirect GroupBy detection */
  CollIndex         keyColumns_;
  CollIndex         reducedConnections_;
  /** 3-part name of the table. */
  const NAString&   name_;
  /** Pointer to table element from the descriptor. */
  const QRTablePtr  tableElement_;
  /** Is this a hub table? */
  NABoolean	    isHub_;
  /** List of equi-join predicates involving this table. */
  EqualitySetList   predList_;
  /** A pointer to this table's BaseTableDetails object. */
  const BaseTableDetailsPtr baseTable_;
  /** Does the join graph contain more than one instance of this table? */
  NABoolean         isSelfJoinTable_;
}; // class JoinGraphTable

/**
 * A "Half Predicate" is an expression that can be found in either side 
 * of the equal sign in an equi-join predicate. It holds an expression
 * text and a pointer to a table that includes all the columns used by 
 * the expression. The expression is typically a name of a single column, 
 * but if it is more complex, all the columns it uses must be of the same
 * table.
 *
 * @see QRJoinGraph
 * @see JoinGraphTable
 * @see JoinGraphEqualitySet
 * @see QRJoinSubGraph
 *****************************************************************************
*/
class JoinGraphHalfPredicate : public NAIntrusiveSharedPtrObject
{
public:
  /**
   * Public constructor.
   * @param tablePtr The table this half-pred is pointing to.
   * @param expr     The half-pred expression (or column name).
   * @param id       The element ID from the MV/query descriptor.
   * @param heap     Heap from which to allocate internal data members.
   */
  JoinGraphHalfPredicate(const JoinGraphTablePtr tablePtr, 
                         const QRElementPtr      elementPtr,
			 const NAString&	 expr,
			 ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
     ,tablePtr_(tablePtr)
     ,elementPtr_(elementPtr)
     ,expr_(expr, heap)
  {}

  /** Get the table this half-pred is pointing to. */
  const JoinGraphTablePtr getTable() const { return tablePtr_; }
  /** Get the half-pred expression. */
  const NAString& getExpr() const { return expr_; }
  /** Get the descriptor ID. */
  const NAString& getID() const { return elementPtr_->getID(); }
  /** Get the descriptor element */
  const QRElementPtr getElementPtr() const { return elementPtr_; }

  /**
   * Match this halfPred to other, which is from a different join graph. 
   * Match only the expression text, not the destination table.
   * @param other The halfPred from the other join graph.
   * @return TRUE if matching, FALSE otherwise.
   */
  NABoolean match(JoinGraphHalfPredicatePtr other) const
  {
    return getExpr() == other->getExpr();
  }

private:
  // Copy construction/assignment not defined.
  JoinGraphHalfPredicate(const JoinGraphHalfPredicate&);
  JoinGraphHalfPredicate& operator=(const JoinGraphHalfPredicate&);

private:
  const JoinGraphTablePtr tablePtr_;
  const NAString	  expr_;
  const QRElementPtr      elementPtr_;
}; // class JoinGraphHalfPredicate

/**
 * An equality set is a list of "half predicates", that are all declared 
 * as equal by the query's equi-join predicates.
 *
 * @see QRJoinGraph
 * @see JoinGraphTable
 * @see JoinGraphHalfPredicate
 * @see QRJoinSubGraph
 *****************************************************************************
*/
class JoinGraphEqualitySet : public NAIntrusiveSharedPtrObject
{
public:
  /**
   * Public constructor.
   * @param id    The equi-join predicate element ID from the MV/query descriptor.
   * @param heap  Heap from which to allocate internal data members.
   */
  JoinGraphEqualitySet(const NAString& id,
                       ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
     ,halfPreds_(heap)
     ,descriptorID_(id, heap)
     ,maxExprSize_(0)
//     ,tempNumber_(-1)
     ,isOnKey_(FALSE)
     ,keyDirection_(0)
  {}

  virtual ~JoinGraphEqualitySet()
  {
    // Delete all the half preds.
    JoinGraphHalfPredicatePtr pred; 
    while (halfPreds_.entries() > 0)
    {
      CollIndex last = halfPreds_.entries()-1;
      pred = halfPreds_[last];
      halfPreds_.removeAt(last);
      deletePtr(pred);
      pred = NULL;
    }
  }

  /** Get the descriptor ID. */
  const NAString& getID() { return descriptorID_; }
  /** Get the list of half-preds in this equality set. */
  HalfPredicateList& getHalfPredicates() { return halfPreds_; }
  
//  Temp numbers are not used for now, because equality set lines 
//  in the MVMemo hash key are not numbered. This may change...
//  int getTempNumber() { return tempNumber_; }
//  void setTempNumber(int ix)  { tempNumber_ = ix; }

  /** Add a half-pred to the equality set. */
  void addHalfPredicate(JoinGraphHalfPredicatePtr pred)
  {
    halfPreds_.insert(pred);

    // Maintain the size of the longest column/expression.
    if (pred->getExpr().length() > maxExprSize_)
      maxExprSize_ = pred->getExpr().length();
  }

  /**
   * Union this and other equality sets by adding all the half preds
   * from other into this, and remove them from other.
   */
  void addHalfPredicatesFrom(JoinGraphEqualitySetPtr other);

  /** 
   * Is this equality set part of the current subgraph?
   * @return TRUE if this subgraph has at least two half-predicates
   * that are part of the subgraph, that is, have a tempNumber other
   * than -1.
   */
  NABoolean isInCurrentSubGraph();

  /**
   * Generate the MVMemo hash key line for this equality set:
   * \par
   * 1. Collect the list of half predicates on participating tables.\par
   * 2. Format them as <table-temp-number>.<column-name/expression> \par
   * 3. Sort them. \par
   * 4. Generate a single line in the format: <tt>{1.CUSTOMER_ID,2.ORDERED_BY}</tt> \par
   * The result is returned in an NAString object allocated off the heap.
   * @return the formatted hash key line.
   */
  NAString* getHashKeyLine(CollHeap* heap);

  /** 
   * Is this equality set directly connected to the sub-graph. 
   */
  NABoolean isConnectedTo(const QRJoinSubGraphPtr subGraph) const;

  /** 
   * Find the halfPred pointing to sourceTable (must be from the same join graph). 
   */
  const JoinGraphHalfPredicatePtr findHalfPredTo(const JoinGraphTablePtr sourceTable) const;

  /**
   * Sets this equality set as a join predicate on a key column of the target table.
   * @param targetTable 
   */
  void setOnKeyTo(const JoinGraphTablePtr targetTable);

  /**
   * Is this equality set a joinpredicate on the target table's key column?
   * @param targetTable 
   * @return 
   */
  NABoolean isOnKeyTo(const JoinGraphTablePtr targetTable);

  /**
   * Get the other table involved in this equality set, that is not the one
   * involved in the given half-pred.
   * @param target 
   * @return 
   */
  JoinGraphTablePtr getOtherTable(JoinGraphHalfPredicatePtr target);

  /**
   * Get the half pred that is on the other side - not on key.
   * @return 
   */
  JoinGraphHalfPredicatePtr getForeignKeySide();

  /**
   * The degree of the equality set is the number of join preds it covers.
   * Since a join pred is from one table to all other tables in the equality set,
   * the degree is equal to the number of halfPreds minus one.
   * @return 
   */
  Int32 getDegree()
  {
    return halfPreds_.entries() - 1;
  }

  Int32 getKeySource()
  {
    if (keyDirection_ == 0)
      return 1;
    else
      return 0;
  }

  /** 
   * Prepare the input text for the Dotty graphical graph viewer for this
   * equality set.
   * @param[out] to Output string.
   */
  void dumpGraph(NAString& to);

private:
  // Copy construction/assignment not defined.
  JoinGraphEqualitySet(const JoinGraphEqualitySet&);
  JoinGraphEqualitySet& operator=(const JoinGraphEqualitySet&);

private:
  HalfPredicateList   halfPreds_;
  const NAString      descriptorID_;
  UInt32        maxExprSize_;
//  int		      tempNumber_;
  NABoolean           isOnKey_;
  Int32                 keyDirection_;
}; // class JoinGraphEqualitySet

/**
 * A join graph is a graph of tables and equi-join predicates, corresponding 
 * to a JBB of an MV or query.
 * The nodes of the graph are the tables and the equality sets, while the 
 * edges are the half predicates.
 * The QRJoinGraph class encapsulates the methods for building the graph
 * from the descriptor classes, while the traversal of the graph is the 
 * responsibility of the HubIterator class.
 *
 * @see JoinGraphTable
 * @see JoinGraphHalfPredicate
 * @see JoinGraphEqualitySet
 * @see QRJoinSubGraph
 * @see HubIterator
 *****************************************************************************
*/
class QRJoinGraph : public NAIntrusiveSharedPtrObject
{
  friend class QRJoinSubGraph;

public:
  /**
   * Public constructor.
   * @param heap  Heap from which to allocate internal data members.
   */
  QRJoinGraph(ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
     ,heap_(heap)
     ,nextTable_(0)
     ,isSelfJoin_(FALSE)
     ,tableHashByID_(hashKey, INIT_HASH_SIZE_SMALL, TRUE, heap)
     ,equalitySets_(heap)
     ,tableArray_(heap, 10)
     ,name_(heap)
     ,hasGroupBy_(FALSE)
     ,hubSize_(0)
     ,tablesToReduce_(heap_)
     ,minimizedSubGraph_(NULL)
     ,sortedTables_(NULL)
     ,pos_(0)
  {}

  virtual ~QRJoinGraph();

  // Accessors
  ////////////////////

  /** Does this join graph include more than one instance of any table? */
  NABoolean isSelfJoin() { return isSelfJoin_; }
  /** Get the heap pointer. */
  CollHeap* getHeap() { return heap_; }
  /** How many tables are included in this join graph? (For self-joins, individual instances are counted) */
  CollIndex entries() { return tableArray_.entries(); }
  /** Mark that this join graph includes a GROUP BY node. */
  void setGroupBy() { hasGroupBy_ = TRUE; }
  /** Does this join graph include a GROUP BY node at the top? */
  NABoolean hasGroupBy() { return hasGroupBy_; }
  /** How many tables are in the hub? */
  Int32 getHubSize() const { return hubSize_; }

  /**
   * Find the JoinGraphTable object from the table ID.
   * @param id The descriptor ID of the table.
   * @return Pointer to JoinGraphTablePtr object.
   */
  JoinGraphTablePtr getTableByID(const NAString& id)
  {
    return tableHashByID_.getFirstValue(&id);
  }

  /** Find an equality set using its descriptor ID. */
  JoinGraphEqualitySetPtr FindEqualitySetByID(const NAString& id);

  /**
   * Find the JoinGraphTable object from the table's ordinal number.
   * @param ix The table's ordinal number - its index into the QRJoinGraph table array.
   * @return Pointer to JoinGraphTablePtr object.
   */
  JoinGraphTablePtr getTableByOrdinal(CollIndex ix)
  {
    return tableArray_[ix];
  }

  /**
   * Get the list of equality sets in this join graph.
   */
  EqualitySetList& getEqualitySets() { return equalitySets_; }

  /**
   * Get the number of tables in this join graph.
   * @return 
   */
  CollIndex getNumberOfTables()
  {
    return nextTable_;
  }

  JoinGraphTableList& getTablesToReduce()
  {
    return tablesToReduce_;
  }

  QRJoinSubGraphPtr getMinimizedSubGraph()
  {
    return minimizedSubGraph_;
  }

  // Graph construction methods
  /////////////////////////////////

  /**
   * Add a table to the join graph.
   * @param tableElement A table element from the descriptor.
   */
  void addTable(const QRTablePtr	  tableElement, 
		NABoolean		  isHub, 
		const BaseTableDetailsPtr baseTable);

  /**
   * Add a half-pred to the join graph.
   * @param equalitySet Pointer to the equality set object.
   * @param tablePtr    Pointer to the table object.
   * @param expr        The half-pred expression.
   * @param id          The descriptor ID of the new half-pred.
   */
  void addHalfPredicate(JoinGraphEqualitySetPtr   equalitySet,
			JoinGraphTablePtr	  tablePtr,
                        const QRElementPtr        elementPtr,
			const NAString&		  expr);

  /**
   * Add an equality set to the join graph from the descriptor data.
   * @param predElement The descriptor element for the new equality set.
   */
  void addEqualitySet(const QRJoinPredPtr predElement);

  /**
   * Analyze a JBB element from the descriptor, and build the entire join graph from it.
   * @param jbb 
   */
  void initFromJBB(const QRJBBPtr jbb, MVDetailsPtr mv);

  /**
   * Reset the temp numbers of all tables to -1 before starting a new traversal of the join graph.
   */
  void resetTempNumbers();

  /** A heuristic method for finding a good table to start a connected graph walk. */
  JoinGraphTablePtr getFirstTable();
  /** Find a table that is connected to the subgraph, but not yet part of it. */
  JoinGraphTablePtr getNextTable(QRJoinSubGraphPtr subGraph);

  // Debugging stuff
  ////////////////////

  /** Set a name to the join graph for the graphical representation. */
  void setName(const NAString& name) { name_ = name; }
  /** Get the name of the join graph. */
  const NAString& getName() { return name_; }
  /** Create a graphical representation of the join graph by using the DOT language. */
  void dumpGraph(MVDetailsPtr mv);

private:
  void shiftTable(CollIndex insertPos);
  CollIndex shiftArray(const NAString name);

private:
  // Copy construction/assignment not defined.
  QRJoinGraph(const QRJoinGraph&);
  QRJoinGraph& operator=(const QRJoinGraph&);

private:
  CollHeap*	      heap_;
  Int32		      nextTable_;
  NABoolean	      isSelfJoin_;
  JoinGraphTableHash  tableHashByID_;
  EqualitySetList     equalitySets_;
  JoinGraphTableArray tableArray_;
  NAString            name_; // For debugging
  NABoolean	      hasGroupBy_;
  Int32		      hubSize_;
  JoinGraphTableList  tablesToReduce_;
  QRJoinSubGraphPtr   minimizedSubGraph_;
  JoinGraphTableList* sortedTables_;  // Used for WA SQL generation
  UInt32              pos_;
}; // class QRJoinGraph

/**
 * A QRJoinSubGraph includes a subset of the tables of its parent join graph
 * and all the equality sets that include at least two half-predicates on 
 * these tables.
 * for loop sample use:
 *   QRJoinSubGraph sg;
 *   for(sg.reset(); sg.hasNext(); sg.advance()) ... getCurrent()...
 *
 * @see QRJoinGraph
 * @see JoinGraphTable
 * @see JoinGraphHalfPredicate
 * @see JoinGraphEqualitySet
 * @see HubIterator
 *****************************************************************************
 */
class QRJoinSubGraph : public NAIntrusiveSharedPtrObject
{
public:
  /**
   * Public constructor.
   * @param parent A pointer to the join graph this subgraph is a part of.
   * @param heap   Heap from which to allocate internal data members.
   */
  QRJoinSubGraph(QRJoinGraphPtr parent, ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
     ,parentGraph_(parent)
     ,subArray_(&parent->tableArray_, heap)
     ,graphHashKey_(heap)
     ,pos_(0)
     ,isSelfJoin_(FALSE)
     ,hasGroupBy_(FALSE)
     ,stillUsed_(FALSE)
     ,fastKey_(heap)
     ,selfJoinHandler_(NULL)
  {}
  
  /**
   * Copy construction IS defined for this class.
   */
  QRJoinSubGraph(const QRJoinSubGraph& other, ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
     ,parentGraph_(other.parentGraph_)
     ,subArray_(other.subArray_)
     ,graphHashKey_(heap)
     ,pos_(0)
     ,isSelfJoin_(other.isSelfJoin_)
     ,hasGroupBy_(other.hasGroupBy_)
     ,stillUsed_(FALSE)
     ,fastKey_(heap)
     ,selfJoinHandler_(NULL)
  {}

  virtual ~QRJoinSubGraph()
  {}

  // Mutators

  /** Add a table to the join subgraph. */
  void addTable(JoinGraphTablePtr table);

  /** Remove a table from this join subgraph. */
  void removeTable(JoinGraphTablePtr table);

  /** Add all the tables in the join graph to this subgraph. */
  void addAllTables();

  /** Does this subgraph contain that table? */
  NABoolean contains(JoinGraphTablePtr table)
  {
    return subArray_.contains(table->getOrdinalNumber());
  }

  /** Return the pointer to the parent JoinGraph. */
  QRJoinGraphPtr getParentGraph()
  {
    return parentGraph_;
  }

  /** equals operator. */
  NABoolean operator==(const QRJoinSubGraph& other)
  {
    return subArray_   == other.subArray_ &&
           hasGroupBy_ == other.hasGroupBy_;
  }

  void updateTables(const QRJoinSubGraphPtr other)
  {
    subArray_ = other->subArray_;
  }

  /** Mark that this subgraph includes a GROUP BY node. */
  void setGroupBy() 
  { 
    hasGroupBy_ = TRUE; 
    graphHashKey_ = ""; // Invalidate hash key.
    fastKey_ = "";      // Invalidate hash key.
  }

  /** Does this subgraph include a GROUP BY node? */
  NABoolean hasGroupBy() { return hasGroupBy_; }

  // Mark that this subGraph is being used for the rewrite instructions
  void setStillUsed(NABoolean used)
  { stillUsed_ = used; }
  NABoolean stillUsed()
  { return stillUsed_; }

  /**
   * Compute the MVMemo hash key for the contained sub-graph.
   * @return the hash key text.
   */
  const QRJoinSubGraphMapPtr getSubGraphMap(OperationType op);

  // Methods for iterating over the contained tables
  ///////////////////////////////////////////////////////
  void reset()                   { pos_ = 0; }
  NABoolean hasNext()            { return subArray_.nextUsedFast(pos_); }
  void advance()                 { pos_++; }
  JoinGraphTablePtr getCurrent() { return subArray_.element(pos_); }
  CollIndex entries()		 { return subArray_.entries(); }

  /**
   * Calculate the "fastKey" - a quick internal hash key based on the used tables 
   * and groupby flag. Internal means that it uses the table IDs, so can only 
   * be used for comparisons on the same descriptor.
   */
  void calcFastKey();

  /**
   * Get the FastKey hash string.
   * @return 
   */
  const NAString* getFastKey()
  {
    // If we have not already calculated the fastKey, do it now.
    if (fastKey_ == "")
      calcFastKey();

    return &fastKey_;
  }

  /**
   * Is the subGraph using all the hub tables in the full join graph?
   * @return 
   */
  NABoolean isFull() const
  {
    return subArray_.entries() == parentGraph_->getHubSize() &&
	   hasGroupBy_ == parentGraph_->hasGroupBy();
  }

  /**
   * Does this subgraph contain a self-join?
   * @return 
   */
  NABoolean isSelfJoin()
  {
    return isSelfJoin_;
  }

  void setSelfJoin()
  {
    isSelfJoin_ = TRUE;
  }

  /**
   * 
   * @return 
   */
  SelfJoinHandlerPtr  getSelfJoinHandler()
  {
    return selfJoinHandler_;
  }

  void doEqualitySetsPart(NAString& hashKey) const;

protected:
  void doTablesPart(NAString& hashKey, QRJoinSubGraphMapPtr map);

  // Assignment NOT defined for this class.
  QRJoinSubGraph& operator=(const QRJoinSubGraph&);

private:
  QRJoinGraphPtr      parentGraph_;
  JoinGraphTableSubArray     subArray_;
  NAString	      graphHashKey_;
  CollIndex	      pos_;
  NABoolean	      isSelfJoin_;
  NABoolean	      hasGroupBy_;
  NABoolean	      stillUsed_;
  NAString	      fastKey_;
  SelfJoinHandlerPtr  selfJoinHandler_;
}; // class QRJoinSubGraph

/**
 * 
 * 
 * 
 * 
 * 
 */
class QRJoinSubGraphMap : public NAIntrusiveSharedPtrObject
{
public:
  /**
   * Public constructor.
   * @param parent A pointer to the join graph this subgraph is a part of.
   * @param heap   Heap from which to allocate internal data members.
   */
  QRJoinSubGraphMap(QRJoinSubGraphPtr subGraph, NABoolean isMV, CollIndex size, 
		    ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
     ,subGraph_(subGraph)
     ,hashKey_(heap)
     ,tableMapping_(heap, size)
     ,indexHashByID_(hashKey, INIT_HASH_SIZE_SMALL, TRUE, heap) // Pass NAString::hashKey
     ,isMV_(isMV)
     ,mvDetails_(NULL)
     ,isACopy_(FALSE)
     ,shiftVector_(heap)
  {
    subGraph->setStillUsed(TRUE);
  }

  // Copy Ctor.
  QRJoinSubGraphMap(const QRJoinSubGraphMap& other, ADD_MEMCHECK_ARGS_DECL(CollHeap* heap));

  virtual ~QRJoinSubGraphMap();

  void addTable(CollIndex ix, const JoinGraphTablePtr table);

  /**
   * The query side translation: find the hash key index for a particular table.
   * @param id The query descriptor ID of the table.
   * @return The query hash key index, -1 if not found.
   */
  Int32 getIndexForTable(const NAString& id) const;
  
  /**
   * The MV side translation: find the table information for its index.
   * @param ix The table index in the MV hash key.
   * @return Pointer to the table information.
   */
  const JoinGraphTablePtr getTableForIndex(CollIndex ix) const;

  /**
   * Is the subGraph using all the tables in the full joinn graph?
   * @return 
   */
  NABoolean isFull() const
  {
    return subGraph_->isFull();
  }

  void setHashKey(const NAString& key)
  {
    hashKey_ = key;
  }

  const NAString& getHashKey()
  {
    return hashKey_;
  }

  QRJoinSubGraphPtr getSubGraph()
  { 
    return subGraph_;
  }

  NABoolean contains(const NAString& id)
  {
    return indexHashByID_.contains(&id);
  }

  void setMVDetails(MVDetailsPtr mv)
  {
    mvDetails_ = mv;
  }

  MVDetailsPtr getMVDetails() const
  {
    return mvDetails_;
  }

  /** Does this subgraph include a GROUP BY node? */
  NABoolean hasGroupBy() { return subGraph_->hasGroupBy(); }

  // Self-Joins
  ///////////////////

  /**
   * Does the join subgraph this map is based on include more than 
   * a single instance of any of its tables?
   */
  NABoolean isSelfJoin() 
  { 
    return subGraph_->isSelfJoin(); 
    // Return FALSE here in order to disable SelfJoin handling.
    //return FALSE;
  }

  void prepareForSelfJoinWork();

  /**
   * Does the last subgraph have an equivalent MVMemo hash key for its self-join?
   */
  NABoolean hasNextEquivalentMap() const;

  /**
   * Get the next equivalent MVMemo hash key for the current self-join subgraph.
   */
  QRJoinSubGraphMapPtr nextEquivalentMap() const;

  const NAString& getShiftVector()
  {
    return shiftVector_;
  }

  void restoreTempNumbers() const;

private:
  class UnsignedInteger : public NABasicObject
  {
  public:
    // Default constructor
    UnsignedInteger()
      : data_(0)
    {}

    UnsignedInteger(CollIndex data)
      : data_(data)
    {}

    virtual ~UnsignedInteger()
    {}

    // Copy construction
    UnsignedInteger(const UnsignedInteger& other)
      : data_(other.data_)
    {}

    // Assignment operator
    UnsignedInteger& operator=(const UnsignedInteger& other)
    { data_ = other.data_; return *this; }

    NABoolean operator==(const UnsignedInteger& other)
    { return data_ == other.data_; }

    CollIndex getInt()
    { return data_; }

  private:
    CollIndex data_;
  }; // Internal class UnsignedInteger

  typedef NAHashDictionary<const NAString, UnsignedInteger>	    IndexHash;
  typedef NAHashDictionaryIterator<const NAString, UnsignedInteger> IndexHashIterator;

  QRJoinSubGraphPtr   subGraph_;
  NAString	      hashKey_;
  JoinGraphTableArray tableMapping_;
  IndexHash	      indexHashByID_;
  NABoolean	      isMV_;
  MVDetailsPtr        mvDetails_;
  NABoolean	      isACopy_;  // Was this object created using the copy constructor?
  NAString            shiftVector_; // For debugging only.
};  // class QRJoinSubGraphMap

/**
 * Takes a join graph, generates a stream of unique subgraphs of it, and 
 * produces the hash keys of those subgraphs.
 * For now, Produces only connected subgraphs - graphs with cross products 
 * are not handled yet.
 * Has two modes of operation: for insert and search operations in MVMemo.
 *
 * @see QRJoinGraph
 * @see QRJoinSubGraph
 * @see MVMemo
 * \todo Produce equivalent expressions for self-joins.
 * \todo Produce more expressions for join graphs with circular join predicates.
 *****************************************************************************
 */
class HubIterator : public NAIntrusiveSharedPtrObject
{
public:
  /**
   * Public constructor.
   * @param joinGraph    The join graph to iterate on.
   * @param isForInsert  Operation mode: insert or search?
   * @param heap         Heap from which to allocate internal data members.
   */
  HubIterator(QRJoinGraphPtr   joinGraph, 
	      OperationType    isForInsert, 
	      ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
     ,joinGraph_(joinGraph)
     ,opType_(isForInsert)
     ,subGraphsVisited_(hashKey, INIT_HASH_SIZE_LARGE, FALSE, heap) 
     ,subGraphsReady_(heap, 100)
     ,subGraphsToAvoid_(hashKey, INIT_HASH_SIZE_LARGE, FALSE, heap)
     ,currentSubgraph_(NULL)
     ,usedGroupBy_(FALSE)
  {}

  virtual ~HubIterator();

  /**
   * There are probably more results if there are still more subgraphs in 
   * the ready list. Its only probably true because the remaining subgraphs 
   * may be duplicates. However, if this method returns FALSE, then its final.
   */
  NABoolean hasNext() 
  {
    assertLogAndThrow(CAT_MVMEMO_JOINGRAPH, LL_MVQR_FAIL,
                      subGraphsVisited_.entries() > 0, QRLogicException, 
		      "This method should not be called before trying any subgraphs.");
    
    // Check if we have subgraphs to work on.
    if (subGraphsReady_.entries() > 0)
      return TRUE;

    // Did we check the top subgraph with the GroupBy flag yet?
    if (joinGraph_->hasGroupBy() && !usedGroupBy_)
      return TRUE;

    return FALSE;
  }

  /** 
   * Initialize the iterator: populate the ready list with all the single-table
   * subgraphs of the join graph.
   */
  void init();

  /** 
   * Return the next subgraph to search/insert.
   */
  const QRJoinSubGraphPtr nextSubGraph();

  const QRJoinSubGraphPtr getMinimizedSubGraph();

  /** 
   * Computes the supersets of subGraph that include a single additional table
   * that is conneted to it, and add them to the ready list. \par
   * 1. For each table included in the subgraph, find all the connected tables
   *    that are not yet part of the subgraph, and add them to a set of 
   *    unique entries. \par
   * 2. For each of the tables in the resulting set, create a new subgraph from 
   *    the current subgraph plus that table, and add it to the ready list. \par
   * @param subGraph The current subgraph.
   */
  void getNextLevelSupersets(QRJoinSubGraphPtr subGraph);

  /** 
   * The current join graph uses an MV as one of its tables. 
   * Rewrite the join graph to use the MVs query instead. 
   */
  void rewriteJoinGraph(const MVDetailsPtr mvDetails) {}

  /**
   * During a search operation, report that the current subgraph was not found
   * in MVMemo, and so it and its supersets should be avoided.
   */
  void reportAsMissing();

  /**
   * Does this join graph have too many self-joins?
   * @return 
   */
  NABoolean isSelfJoinTooBig();

  /**
   * Create a subGraph for the full join graph.
   * @return 
   */
  const QRJoinSubGraphPtr getFullSubGraph();

private:
  // Copy construction/assignment not defined.
  HubIterator(const HubIterator&);
  HubIterator& operator=(const HubIterator&);

private:
  // Max threshold for self-join permutation work.
  // If the join graph is more complex than this, MVMemo will skip self-join work.
  // and only same order queries will be matched.
  // Biggest Yotta query I've seen is 4x4x2 = 1,152
  // At 8x2 > 80,000 we run out of memory and crash.
  // 6000 allows for a 5x4x2 self-join (5760).
  enum { MAX_SELFJOIN_PERMUTATIONS = 6000 };

  QRJoinGraphPtr      joinGraph_;
  OperationType	      opType_;

  /** This hash table remembers every subgraph we tried. Used to detect duplicates. */
  SubGraphHash	      subGraphsVisited_;

  /** The ready list contains subgraphs that are ready to be tested. */
  SubGraphList	      subGraphsReady_;

  /** Subgraphs that were not found in MVMemo during search, and so they and their supersets should be skipped. */
  SubGraphHash	      subGraphsToAvoid_;

  /** The last subgraph that was returned. */
  QRJoinSubGraphPtr   currentSubgraph_;

  // Did the iterator already produce the subgraph with the GROUP BY node?
  NABoolean	      usedGroupBy_;
};  // class HubIterator


#endif
