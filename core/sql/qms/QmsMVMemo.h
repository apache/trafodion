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

/***********************************************************************
 *
 * File:         MVMemo.h
 * Description:  Implementation of the MVMemo data structure for matching 
 *               a query JBB to JBBs of candidate MVs.
 *
 * Created:      12/04/07
 ***********************************************************************/

// _MEMSHAREDPTR;_MEMCHECK

#ifndef _MVMEMO_H_
#define _MVMEMO_H_


#include "NABasicObject.h"
#include "NAString.h"
#include "Collections.h"
#include "QRSharedPtr.h"
#include "QRDescriptor.h"

class MVMemo;
class MVMemoExpression;
class MVMemoLogicalExpression;
class MVMemoPhysicalExpression;
class MVMemoGroup;
class HubIterator;

#ifdef _MEMSHAREDPTR
typedef QRIntrusiveSharedPtr<MVMemo>			MVMemoPtr;
typedef QRIntrusiveSharedPtr<MVMemoLogicalExpression>	MVMemoLogicalExpressionPtr;
typedef QRIntrusiveSharedPtr<MVMemoPhysicalExpression>	MVMemoPhysicalExpressionPtr;
typedef QRIntrusiveSharedPtr<MVMemoGroup>		MVMemoGroupPtr;
typedef QRIntrusiveSharedPtr<HubIterator>		HubIteratorPtr;
#else
typedef MVMemo*			  MVMemoPtr;
typedef MVMemoLogicalExpression*  MVMemoLogicalExpressionPtr;
typedef MVMemoPhysicalExpression* MVMemoPhysicalExpressionPtr;
typedef MVMemoGroup*		  MVMemoGroupPtr;
typedef HubIterator*		  HubIteratorPtr;
#endif

// !!! These collection types do NOT inherit from NAIntrusiveSharedPtrObject themselves!!!
typedef NAPtrList<MVMemoGroupPtr>				             MVMemoGroupList;
typedef SharedPtrValueHash<const NAString, MVMemoLogicalExpression>          MVMemoExpressionHash;
typedef SharedPtrValueHashIterator<const NAString, MVMemoLogicalExpression>  MVMemoExpressionHashIterator;
typedef SharedPtrValueHash<const NAString, MVMemoPhysicalExpression>         GroupExpressionHash;
typedef SharedPtrValueHashIterator<const NAString, MVMemoPhysicalExpression> GroupExpressionHashIterator;

#include "QmsJoinGraph.h"
#include "QmsMVCandidate.h"
#include "QmsGroupLattice.h"
#include "QmsWorkloadAnalysis.h"


/**
 * Class MVMemoLogicalExpression
 * Implements an MVMemo logical expression
 * @see MVMemoPhysicalExpression
 * @see MVMemoGroup
 * @see MVMemo
 *****************************************************************************
 */
class MVMemoLogicalExpression : public NAIntrusiveSharedPtrObject
{
public:
  /**
   * Class constructor
   * @param key The hash key for the new expression.
   * @param groupNumber The group number this expression will belong to
   */
  MVMemoLogicalExpression(const NAString& key, Int32 groupNumber, ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
     ,hashKey_(key, heap)
     ,groupNumber_(groupNumber)
  {}

  virtual ~MVMemoLogicalExpression() {}

  NABoolean operator==(const MVMemoLogicalExpression& other)
  {
    return !hashKey_.compareTo(other.hashKey_); 
  }

  /**
   * Virtual method for checking if an MVMemo expression is physical.
   * @return FALSE
   */
  virtual NABoolean isPhysical() { return FALSE; }

  /**
   * @return the group number 
   */
  Int32 getGroupNumber() { return groupNumber_; }

  /**
   * @return the hash key
   */
  const NAString& getHashKey() { return hashKey_; }

private:
    // Copy construction/assignment not defined.
    MVMemoLogicalExpression(const MVMemoLogicalExpression&);
    MVMemoLogicalExpression& operator=(const MVMemoLogicalExpression&);

private:
  const NAString  hashKey_;
  const Int32	  groupNumber_;
};  // class MVMemoLogicalExpression


/**
 * Class MVMemoPhysicalExpression
 * Implements an MVMemo physical expression.
 * Stores an MVDetails object of a matching candidate MV.
 * @see MVMemoLogicalExpression
 * @see MVMemoGroup
 * @see MVMemo
 *****************************************************************************
 */
class MVMemoPhysicalExpression : public MVMemoLogicalExpression
{
public:

  /**
   * Class constructor
   * @param key The hash key for the new expression.
   * @param groupNumber The group number this expression will belong to
   * @param mv The name of the MV.
   */
  MVMemoPhysicalExpression(const QRJoinSubGraphMapPtr map, 
			   Int32 groupNumber, 
			   const MVDetailsPtr mv, 
			   ADD_MEMCHECK_ARGS_DECL(CollHeap* heap));

  virtual ~MVMemoPhysicalExpression();

  /**
   * Virtual method for checking if an MVMemo expression is physical.
   * @return TRUE - override superclass implementation.
   */
  virtual NABoolean isPhysical() { return TRUE; }

  /**
   * 
   * @return the details of the MV
   */
  const MVDetailsPtr getMVDetails() { return mv_; }

  /**
   * 
   * @return the name of the MV
   */
  const NAString& getMVName();

  /**
   * 
   * @return 
   */
  const QRJoinSubGraphMapPtr getMvSubGraphMap()
  {
    return map_;
  }

private:
  // Copy construction/assignment not defined.
  MVMemoPhysicalExpression(const MVMemoPhysicalExpression&);
  MVMemoPhysicalExpression& operator=(const MVMemoPhysicalExpression&);

private:
  const MVDetailsPtr	      mv_;
  const QRJoinSubGraphMapPtr  map_;
}; // class MVMemoPhysicalExpression

/**
 * Class MVMemoGroup
 * Includes a list of logical expressions and a list of physical expressions.
 * Every group has a unique group number.
 * @see MVMemoLogicalExpression
 * @see MVMemoPhysicalExpression
 * @see MVMemo
 *****************************************************************************
 */
class MVMemoGroup : public NAIntrusiveSharedPtrObject
{
public:
 /**
  * Class constructor
  * @param number The MVMemo group number
  * @param heap A heap pointer for internal allocations.
  */
  MVMemoGroup(Int32 number, ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
     ,groupNumber_(number)
     ,logicalExprs_(heap)
     ,physicalExprsHash_(hashKey, INIT_HASH_SIZE_SMALL, TRUE, heap) // Pass NAString::hashKey
     ,groupLattice_(NULL)
     ,tempExpr_(NULL)
  {}

  /**
   * Class destructor
   */
  virtual ~MVMemoGroup() 
  {
    if (groupLattice_)
      deletePtr(groupLattice_);
  }

  /**
   * @return The group number.
   */
  Int32 getGroupNumber()
  {
    return groupNumber_;
  }

  /**
   * Does this group have any physical expressions in it?
   */
  NABoolean hasPhysicalExpressions()
  {
    return (physicalExprsHash_.entries() > 0);
  }

 /**
  * Add the MVs in this group to the list of candidates so far.
  */
  void getPhysicalExpressions(MVCandidatesForJBBPtr   candidateList, 
			      QRJoinSubGraphMapPtr    querySubGraphMap,
			      QRJBBPtr		      jbb,
			      CollHeap*		      heap, 
			      NABoolean		      isPreferredMatch,
                              ElementPtrList*         minimizedGroupingList);

 /**
  * Add a logical expression to the group's list of logical expressions.
  * @param expr The logical expression to add.
  */
  MVMemoLogicalExpressionPtr addLogicalExpression(const NAString& key, 
						  ADD_MEMCHECK_ARGS_DECL(CollHeap* heap));

 /**
  * Add a physical expression to the group's list of physical expressions.
  */
  NABoolean addPhysicalExpression(const QRJoinSubGraphMapPtr     map, 
			          const MVDetailsPtr		 mv, 
			          const QRJBBPtr		 jbb,
			          NABoolean			 hasGroupby,
			          ADD_MEMCHECK_ARGS_DECL(CollHeap* heap));

  /**
   * Remove a physical expression for an MV that has been dropped.
   * @param key The hash key for the MV to remove.
   */
  void removePhysicalExpression(MVMemoPhysicalExpressionPtr expr);

  /**
   * 
   * @param name 
   * @return 
   */
  QRJoinSubGraphMapPtr getSubGraphMapForMV(const NAString& name)
  {
    MVMemoPhysicalExpressionPtr expr = physicalExprsHash_.getFirstValue(&name);
    assertLogAndThrow(CAT_MVMEMO_JOINGRAPH, LL_MVQR_FAIL,
                  expr != NULL, QRLogicException, 
		  "An MV found by GroupLattice must be in physical expression list.");
    return expr->getMvSubGraphMap();
  }

  /**
   * 
   * @param name 
   * @return 
   */
  void dropMV(const NAString& name);

  /**
   * Start renaming an MV being pointed to by this Group.
   * @param oldName 
   * @return 
   */
  void startRenameMV(const NAString& oldName);

  /**
   * Finish renaming an MV being pointed to by this Group.
   * @return 
   */
  void FinishRenameMV();

  /**
   * Set the pointer to the next step of matching algorithm, to further
   * reduce the number of candidate MVs.
   * @param next Pointer to next matching data structure.
   */
  void setGroupLattice(QRGroupLatticePtr lattice) 
  { 
    groupLattice_ = lattice; 
  }

  /**
   * Get the pointer to the next step of matching algorithm, to further
   * reduce the number of candidate MVs.
   * @return Pointer to next matching data structure.
   */
  QRGroupLatticePtr getGroupLattice() 
  { 
    return groupLattice_; 
  }

  Int32 getNumOfMVs();

  void reportStats(NAString& text);

  /**
   * Collect data on query (MV) groups with shared join+GroupBy.
   */
  void collectMVGroups(WorkloadAnalysisPtr workload, Int32 minQueriesPerMV, CollHeap* heap);

private:
    // Copy construction/assignment not defined.
    MVMemoGroup(const MVMemoGroup&);
    MVMemoGroup& operator=(const MVMemoGroup&);

private:
  const Int32				  groupNumber_;
  NAPtrList<MVMemoLogicalExpressionPtr>   logicalExprs_;
  GroupExpressionHash			  physicalExprsHash_;
  QRGroupLatticePtr			  groupLattice_;
  MVMemoPhysicalExpressionPtr             tempExpr_;
};  // class MVMemoGroup

/**
 * Main class of the MVMemo data structure.
 * Implements the insert and search methods.
 * @see MVMemoLogicalExpression
 * @see MVMemoPhysicalExpression
 * @see MVMemoGroup
 * @see HubIterator
 *****************************************************************************
 */
class MVMemo : public NAIntrusiveSharedPtrObject
{
public:
  /**
   * Class constructor
   * @param heap Heap from which to allocate internal objects.
   * @return 
   */
  MVMemo(ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
      ,heap_(heap)
      ,expressionHash_(hashKey, INIT_HASH_SIZE_LARGE, TRUE, heap) // Pass NAString::hashKey
      ,groupsArray_(heap, 100)
      ,nextGroup_(0)
  {}

  /**
   * Class destructor
   * \todo - Delete the lists.
   */
  virtual ~MVMemo();

  void insert(QRJBBPtr jbb, MVDetailsPtr mv);

  void search(QRJBBPtr jbb, MVCandidatesForJBBPtr candidateList, CollHeap* heap);

  HubIteratorPtr createHubIteratorForJBB(const QRJBBPtr jbb, MVDetailsPtr mv, OperationType op, CollHeap* heap);

  /**
   * Get the group with the specified number.
   */
  MVMemoGroupPtr getGroup(Int32 number)
  {
    assertLogAndThrow(CAT_MVMEMO_JOINGRAPH, LL_MVQR_FAIL,
                      number < nextGroup_, QRLogicException, 
		      "Invalid MVMemo group number.");
    return groupsArray_[number];
  }

  /**
   * Allocate a new MVMemo group, and insert it into the MVMemo group array.
   * @return A pointer to the new group.
   */
  MVMemoGroupPtr allocateNewGroup()
  {
    MVMemoGroupPtr newGroup = new(heap_) 
      MVMemoGroup(nextGroup_++, ADD_MEMCHECK_ARGS(heap_));
    groupsArray_.insertAt(newGroup->getGroupNumber(), newGroup);
    return newGroup;
  }

  /**
   * Does MVMemo already contain a group with the specified hash key?
   * @param  key Hash key of group.
   * @return TRUE if found, FALSE otherwise.
   */
  NABoolean contains(const NAString& key) 
  { 
    return expressionHash_.contains(&key); 
  }

  /**
   * Get the logical or physical expression with the specified hash key.
   * @param key Hash key of expression
   * @return Pointer to found expression.
   */
  MVMemoLogicalExpressionPtr getExpression(const NAString& key)
  {
    return expressionHash_.getFirstValue(&key);
  }

  /**
   * Insert the specified expression into the expression hash table.
   * @param expr 
   */
  void insertExpression(MVMemoLogicalExpressionPtr expr)
  {
    expressionHash_.insert(&expr->getHashKey(), expr);
  }

  NABoolean handleInsertedSubgraph(QRJoinSubGraphMapPtr map, 
                                   QRJBBPtr jbb, 
                                   MVDetailsPtr mv,
                                   NABoolean& isDuplicate);

  MVMemoGroupPtr probeForSubGraph(HubIteratorPtr hubIterator, 
                                  const QRJoinSubGraphMapPtr map);

  /**
   * Report usage statistics to the log file.
   */
  void reportStats(Int32 numOfMVs);

  /**
   * Collect data on query (MV) groups with shared join+GroupBy.
   */
  void collectMVGroups(WorkloadAnalysisPtr workload, Int32 minQueriesPerMV, CollHeap* heap);

private:
    // Copy construction/assignment not defined.
    MVMemo(const MVMemo&);
    MVMemo& operator=(const MVMemo&);

private:
  CollHeap*		      heap_;             // Heap pointer from which to allocate internal objects.
  MVMemoExpressionHash	      expressionHash_;   // Hash table of logical and physical expressions.
  NAPtrArray<MVMemoGroupPtr>  groupsArray_;      // Array of groups in MVMemo.
  Int32			      nextGroup_;        // Index of the next group to be inserted.
};  // class MVMemo

/**
 * A helper class for performing the Indirect GroupBy minimization algorithm.
 * The algorithm is trying to push the GroupBy node down through the join, by
 * reducing tables to which the join is done on a unique key. 
 * Grouping columns from this table are transformed to the foreign key columns
 * of the joining table.
 * Reduced tables must have no aggregate functions on them.
 *****************************************************************************
 */
class GroupingListMinimizer : public NAIntrusiveSharedPtrObject
{
public:
  /**
   * Class constructor
   * @param heap Heap from which to allocate internal objects.
   * @return 
   */
  GroupingListMinimizer(const QRJBBPtr jbb, 
                        const QRJoinSubGraphMapPtr map,
                        ADD_MEMCHECK_ARGS_DECL(CollHeap* heap));
 
  /**
   * Class destructor
   */
  virtual ~GroupingListMinimizer()
  {}

  ElementPtrList* calcIndirectGroupingList();

protected:
  typedef NAHashDictionary<const NAString, ElementPtrList>         PartitionedGroupingList;
  typedef NAHashDictionaryIterator<const NAString, ElementPtrList> PartitionedGroupingListIterator;

  NABoolean verifyAggregateFunctions();
  void addGroupingCol(QRElementPtr groupingCol);
  void partitionGroupingList();
  void reduceTable(JoinGraphTablePtr table);
  NABoolean verifyTablesWereReduced();
  ElementPtrList* prepareResult();
  NABoolean isReducedTable(const NAString& tableID);
  void dumpResult(ElementPtrList* result);

private:
  const ElementPtrList&    fullGroupingList_;
  PartitionedGroupingList  partitionedGroupingList_;
  JoinGraphTableList       tablesToReduce_;
  QROutputListPtr          outputs_;
  CollHeap*		   heap_;
};  // class GroupingListMinimizer

#endif // _MVMEMO_H_
