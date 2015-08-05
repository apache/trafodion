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
 * File:         QmsWorkloadAnalysis.h
 * Description:  Implementation of the WorkloadAnalysis and ProposedMV
 *               classes for finding proposed MVs that can be used to 
 *               rewrite queries of existing MVs.
 *               The lingo is a bit confusing, because each query in the
 *               workload is represented here by an MVDetails object,
 *               and the result of the analysis is a ProposedMV.
 *
 * Created:      05/17/11
 ***********************************************************************/

#ifndef _WORKLOADANALYSIS_H_
#define _WORKLOADANALYSIS_H_

#include "NABasicObject.h"
#include "NAString.h"
#include "Collections.h"
#include "QRSharedPtr.h"
#include "QRDescriptor.h"

class WorkloadAnalysis;
class ProposedMV;

#ifdef _MEMSHAREDPTR
typedef QRIntrusiveSharedPtr<ProposedMV>	ProposedMVPtr;
typedef QRIntrusiveSharedPtr<WorkloadAnalysis>	WorkloadAnalysisPtr;
#else
typedef ProposedMV*				ProposedMVPtr;
typedef WorkloadAnalysis*			WorkloadAnalysisPtr;
#endif

typedef NAPtrList<ProposedMVPtr>		ProposedMVPtrList;
typedef SharedPtrValueHash<const NAString, QRElement>          ElementHash;
typedef SharedPtrValueHashIterator<const NAString, QRElement>  ElementHashIterator;
typedef NAHashDictionary<const NAString, const NAString>       QueryNameHash;

#include "QmsMVDetails.h"
#include "QmsMVCandidate.h"

/**
 * The ProposedMV class is initialized with a list of MVDetails objects
 * for queries that share the same join graph and GroupBy list.
 * It is then used to analyze any differences between those queries, 
 * and find the minimal set of predicates that are common to all.
 * Finally, it generates the SQL text for the CREATE MV command.
 ***********************************************************************
 */
class ProposedMV : public NAIntrusiveSharedPtrObject
{
public:
  ProposedMV(ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap)),
      name_(heap),
      mapList_(heap),
      isInitialized_(FALSE),
      groupingColumns_(hashKey, INIT_HASH_SIZE_SMALL, TRUE, heap), // Pass NAString::hashKey
      selectList_(hashKey, INIT_HASH_SIZE_SMALL, TRUE, heap), // Pass NAString::hashKey
      selectArray_(heap),
      rangePredList_(heap),
      residualPredList_(heap),
      mvPairList_(heap),
      canSkipRangePreds_(FALSE),
      canSkipResidualPreds_(FALSE),
      addedOutputs_(heap),
      addedLojTables_(heap),
      hasGroupBy_(FALSE),
      stuffToDelete_(heap),
      heap_(heap)
  {
  }

  virtual ~ProposedMV();

  /**
   * Add a single MVDEtails object to the list.
   * @param mv The MV to add
   */
  void addMV(MVDetailsPtr mv, QRJoinSubGraphMapPtr map);

  /**
   * Add a list of MVDEtails objects.
   * @param mvs The list of MVs to add.
   */
  void addMVs(const SubGraphMapList& maps);

  /**
   * Set the name of the proposed MV, based on its number.
   * @param num The proposed MV number.
   */
  void setName(Int32 num);

  /**
   * Get the name of the proposed MV.
   * @return Get the name of the proposed MV.
   */
  const NAString& getName()
  {
    return name_;
  }

  SubGraphMapList& getMapList()
  {
    return mapList_;
  }

  /**
   * Create the list of MVPair objects.
   * Each MVPair is composed of the first query, and one of the others.
   * So that the first query is compared to all the rest.
   */
  void initializeMVPairList();

  /**
   * Add to the select list all the columns needed by all the MVs.
   */
  void findInclusiveSelectList();

  /**
   * Analyze the range and residual predicates of the MVs to find the
   * subset of common predicates.
   */
  void findReducedPredicateSet();

  /**
   * Generate the SQL for the proposed MV.
   * @param text The NAString to add the SQL text into.
   * @param addComments By default this method adds information on supported
   *                    queries in comment lines. Set this parameter to FALSE
   *                    to return only the SQL command.
   */
  void reportResults(NAString& text, NABoolean addComments = TRUE);

  Int32 getJoinSize();

protected:
  // Methods for analyzing common predicates.
  void initFrom(QRJoinSubGraphMapPtr map);  
  void checkBitmapsFirst();
  void handleRangePredicates();
  void handleResidualPredicates();
  void handleLeftOuterJoins();
  void handleRemovedRangePredicate(QRRangePredPtr pred);
  void handleRemovedResidualPredicate(QRExprPtr pred);
  void addElementToSelectList(const QRElementPtr col);
  void addMvSideColumnToSelectList(const QRColumnPtr mvCol, MVPairPtr thisPair);

  // Methods for generating the CREATE MV SQL.
  void toSQL(NAString& text);
  void unparseSelectClause(NAString& text);
  void unparseFromClause(NAString& text);
  void unparseWhereAndHavingClauses(NAString& text, NAString& havingClause);
  void unparseGroupByClause(NAString& text);
  void unparseRangePreds(NAString& whereClause, NAString& havingClause);
  void unparseResidualPreds(NAString& whereClause, NAString& havingClause);
  NAString getFromClauseForTable(JoinGraphTablePtr table);
  NAString getOnClauseFor(QRJoinSubGraphPtr subGraph, JoinGraphTablePtr nextTable);
  NAString getColumnCorrelationName(QRColumnPtr column);
  NAString unparseColumnOrExpr(const QRElementPtr elem);
  const NAString* getHashKeyForElement(const QRElementPtr elem);
  void addPredicateText(NAString& text, const NAString& pred);
  NABoolean isAddedLojTable(const NAString& id);
  
  NABoolean isSingleQuery() 
  {
    return mapList_.entries() == 1;
  }

private:
  CollHeap*                   heap_;
  NAString                    name_;
  SubGraphMapList             mapList_;
  NABoolean                   isInitialized_;

  // The select list and GroupBy list are both initialized from the first MV, 
  // and then addidional columns may be added (no duplicates allowed).
  // The select array keeps the entries ordered by ordinal number, to avoid sorting.
  ElementHash                 selectList_;
  NAArray<QROutputPtr>        selectArray_;
  ElementHash                 groupingColumns_;

  // The range and residual predicates lists are both initialized from the 
  // first MV, and then non-common predicates are removed.
  NAPtrList<QRRangePredPtr>   rangePredList_;
  NAPtrList<QRExprPtr>        residualPredList_;

  // The MVPair objects are used to match range and residual predicates.
  // Each pair is composed of the first MVDetails, and one of the others.
  MVPairList                  mvPairList_;
  NAPtrList<QROutputPtr>      addedOutputs_;
  StringPtrSet                addedLojTables_;

  NABoolean                   canSkipRangePreds_;
  NABoolean                   canSkipResidualPreds_;
  NABoolean                   hasGroupBy_;

  // Fake elements that are constructed for matching purposes
  // and need to be deleted before destruction.
  NAPtrList<QRElementPtr>     stuffToDelete_;
};  // class ProposedMV

/**
 * The WorkloadAnalysis class is a container of ProposedMV objects.
 ***********************************************************************
 */
class WorkloadAnalysis : public NAIntrusiveSharedPtrObject
{
public:
  WorkloadAnalysis(ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap)),
      proposedMVsList_(heap),
      inventoryHash_(hashKey, INIT_HASH_SIZE_LARGE, TRUE, heap),
      nextMV_(1),
      heap_(heap)
  {
  }

  virtual ~WorkloadAnalysis();

  /**
   * Add a new ProposedMV object to the list.
   * @param pmv
   * @return TRUE if added correctly, FALSE if found to be a duplicate. 
   */
  NABoolean addProposedMV(ProposedMVPtr pmv);

  /**
   * Perform the common predicate analysis.
   */
  void findReducedPredicateSet();

  /**
   * Generate the output text with the SQL text of the proposed MVs.
   * @param ofs The output stream
   */
  void reportResults(ofstream& ofs, Int32 minQueriesPerMV);

  Int32 getMaxJoinSize();

private:
  CollHeap*             heap_;
  ProposedMVPtrList     proposedMVsList_;
  QueryNameHash         inventoryHash_;
  Int32                 nextMV_;
};  // class WorkloadAnalysis

#endif // _WORKLOADANALYSIS_H_
