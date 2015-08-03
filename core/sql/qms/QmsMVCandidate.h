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
// File:         MVCandidate.h
// Description:  
//               
//               
//               
//
// Created:      07/17/08
// ***********************************************************************

// _MEMSHAREDPTR;_MEMCHECK

#include "QRSharedPtr.h"
#include "QRDescriptor.h"
#include "QmsLatticeIndex.h"

// Forward references
class MVCandidate;
class MVPair;
class MVCandidatesForJBBSubset;
class MVCandidatesForJBB;
class MVCandidateCollection;

// Pointer definitions
#ifdef _MEMSHAREDPTR
typedef QRIntrusiveSharedPtr<MVCandidate>		MVCandidatePtr;
typedef QRIntrusiveSharedPtr<MVPair>                    MVPairPtr;
typedef QRIntrusiveSharedPtr<MVCandidatesForJBBSubset>	MVCandidatesForJBBSubsetPtr;
typedef QRIntrusiveSharedPtr<MVCandidatesForJBB>	MVCandidatesForJBBPtr;
typedef QRIntrusiveSharedPtr<MVCandidateCollection>	MVCandidateCollectionPtr;
#else
typedef MVCandidate*					MVCandidatePtr;
typedef MVPair*		                                MVPairPtr;
typedef MVCandidatesForJBBSubset*			MVCandidatesForJBBSubsetPtr;
typedef MVCandidatesForJBB*				MVCandidatesForJBBPtr;
typedef MVCandidateCollection*				MVCandidateCollectionPtr;
#endif

#ifndef _MVCANDIDATE_H_
#define _MVCANDIDATE_H_

#include "QmsMVDetails.h"
#include "QmsJoinGraph.h"
#include "QmsMatchTest.h"

typedef StringPtrSet                                      IDSet;
typedef ElementPtrList				          GroupingList;
typedef NAPtrList<MVCandidatesForJBBSubsetPtr>            JbbSubsetList;
typedef NAPtrList<MVPairPtr>                              MVPairList;
typedef NAHashDictionary<const NAString, const NAString>  TableIDHash;

enum AggregateMatchingType {
  ANT_NO_INIT,	  // Uninitialized.
  AMT_MJV_JQ,	  // MJV with a join query.
  AMT_MJV_AQ,	  // MJV with an aggregate query
  AMT_MAV_AQ_MG,  // MAV with an aggregate query on a matching grouping list.
  AMT_MAV_AQ_DG	  // MAV with an aggregate query on a subset grouping list.
};

/**
 * Contains information about an MV rewrite candidate, including:
 * - The MV details.
 * - The Query details.
 * - The needed matching algorithms.
 * - Data for generating the rewrite instructions.
 *****************************************************************************
 */
class MVCandidate : public NAIntrusiveSharedPtrObject
{
public:
  /**
   * MVCandidate constructor
   * @param mvDetails The MVDetails pointer.
   * @param query The query details
   * @param jbb The pointer to the query JBB descriptor
   * @param jbbSubset The JBBSubset holding this MV candidate.
   * @param heap The heap from which to allocate memory.
   */
  MVCandidate(const MVDetailsPtr	  mvDetails, 
	      const DescriptorDetailsPtr  query,
	      const QRJBBPtr		  jbb,
	      MVCandidatesForJBBSubsetPtr jbbSubset,
	      ADD_MEMCHECK_ARGS_DECL(CollHeap* heap));

  virtual ~MVCandidate() 
  {
    if (disqualifiedReason_)
      delete disqualifiedReason_;
  }

  /**
   * Initialize internal data structures, such as the AggregateMatchingType.
   * @param isPreferredMatch Is this a preferred match.
   * @param groupingList The list of extra grouping columns needed above the MV.
   */
  void init(NABoolean isPreferredMatch, GroupingList* groupingList);

  /**
   * Initialize the AggregateMatchingType.
   */
  void initAggregateMatchingType();

  /**
   * 
   * @param mvMap 
   */
  void setMvSubGraphMap(QRJoinSubGraphMapPtr mvMap) { mvSubGraphMap_ = mvMap; }
  virtual QRJoinSubGraphMapPtr getMvSubGraphMap();
  virtual QRJoinSubGraphMapPtr getQuerySubGraphMap();

  /**
   * Is this a preferred match?
   * @return TRUE if this is a preferred match MV.
   */
  NABoolean isPreferredMatch() {return isPreferredMatch_; }

  /**
   * Get the MVDetails pointer for this candidate.
   * @return MVDetails pointer for this candidate.
   */
  const MVDetailsPtr getMvDetails() { return mvDetails_; }

  /**
   * Get the details of the query
   * @return The details of the query
   */
  const DescriptorDetailsPtr getQueryDetails() { return queryDetails_; }

  /**
   * Get the pointer to the query JBB descriptor
   * @return The pointer to the query JBB descriptor
   */
  const QRJBBPtr getQueryJbb() { return queryJbb_; }

  /**
   * Get the pointer to the query JBB details
   * @return The pointer to the query JBB detail
   */
  JBBDetailsPtr getQueryJbbDetails();

  /**
   * Get the JBBSubset holding this MV candidate
   * @return The JBBSubset holding this MV candidate
   */
  MVCandidatesForJBBSubsetPtr getJbbSubset() { return jbbSubset_; }

  /**
   * Generate the Candidate element of the result descriptor including 
   * the needed rewrite instructions.
   * @return The QRCandidate object of the result descriptor.
   */
  QRCandidatePtr generateDescriptor();

  /**
   * Get the aggregate matching type.
   * @return The aggregate matching type.
   */
  AggregateMatchingType	getAggregateMatchingType() { return matchingType_; }

  /**
   * Run Pass 1 tests.
   * @return FALSE if MV was disqualified, TRUE otherwise.
   */
  NABoolean matchPass1();

  /**
   * Check the extra-hub tables that were found by Pass 1.
   * @return FALSE if MV was disqualified, TRUE otherwise.
   */
  NABoolean CheckExtraHubTables();

  /**
   * Run Pass 2 tests.
   * @return FALSE if MV was disqualified, TRUE otherwise.
   */
  NABoolean matchPass2();

  /**
   * Add new entries to the list of MV extra-hub or back-join tables 
   * used by the query.
   * @param rewrite The rewrite instructions with the table IDs.
   */
  void addExtraHubAndBackJoinTables(RewriteInstructionsItemPtr rewrite);

  /**
   * Add an output column to the output list rewrite instructions.
   * Used to add input columns to expressions.
   * @param rewrite Rewrite instructions for the new output column.
   */
  void addOutputColumn(RewriteInstructionsItemPtr rewrite, NABoolean makeACopy = TRUE);

  /**
   * Add an IDto the list of outputs to avoid.
   * @param id 
   */
  void addOutputToAvoid(const NAString& id)
  {
    outputMatching_.addOutputToAvoid(id);
  }

  /**
   * This MV candidate has been disqualified.
   * Remove it from its JBBSubset, so that it can be deleted.
   */
  void disqualify();

  /**
   * Write to the log file why the MV was disqualified.
   * @param reason Why was the MV disqualified.
   */
  void logMVWasDisqualified(const char* reason);
  void logMVWasDisqualified1(const char* reason, const NAString& arg1);

  /**
   * When an MV is disqualified while checking for optional information,
   * mark it as qualified again.
   */
  void reQualify();

  /**
   * Find the table from the MV descriptor corresponding to the ID of the
   * table from the query descriptor.
   * @param id ID of table from the query descriptor
   * @param assertOnFailure Assert if a corresponding MV table was not found.
   * @return 
   */
  const BaseTableDetailsPtr getMvTableForQueryID(const NAString& id, 
						 NABoolean assertOnFailure = TRUE);

  const BaseTableDetailsPtr getQueryTableForMvID(const NAString& id, 
						 NABoolean assertOnFailure = TRUE);

  /**
   * Set the JBBSubset after adding extra-hub tables.
   * @param jbbSubset 
   */
  void setJbbSubset(MVCandidatesForJBBSubsetPtr jbbSubset)
  {
    jbbSubset_ = jbbSubset;
  }

    /**
   * Set the aggregate matching type.
   */
  void setAggregateMatchingType(AggregateMatchingType type) 
  { 
    matchingType_ = type; 
  }


  NABoolean isAUsedExtraHubTable(const NAString* tableID);

  void cacheTableID(const NAString* tableID, 
                    NABoolean       result, 
                    const NAString* extraHubID);

  NABoolean probeCacheForTableID(const NAString*  tableID, 
                                 NABoolean&       isOutside, 
                                 const NAString*& extraHubID);

  IDSet& getBackJoinTables()
  {
    return backJoinTables_;
  }

  GroupingList*	getExtraGroupingColumns()
  {
    return extraGroupingColumns_;
  }

  NABoolean wasDisqualified()
  {
  	return wasDisqualified_;
  }
  
protected:
  NABoolean CheckAnExtraHubTable(const NAString*      tableID, 
				 QRJoinSubGraphMapPtr mvSubGraphMap,
				 QRJoinSubGraphMapPtr querySubGraphMap,
			         NABoolean&	      extraHubTableWasAdded);

  NABoolean matchPredsFromTableToSubGraph(const QRJoinSubGraphPtr mvSubGraph,
					  const QRJoinSubGraphPtr querySubGraph, 
					  const JoinGraphTablePtr mvGraphTable,
					  const JoinGraphTablePtr queryGraphTable);

  NABoolean matchPredsFromEQSetToSubGraph(const QRJoinSubGraphPtr	   mvSubGraph, 
					  const QRJoinSubGraphPtr	   querySubGraph, 
					  const JoinGraphEqualitySetPtr    mvEqSet,
					  const JoinGraphEqualitySetPtr    queryEqSet,
					  const NAString&		   mvHalfPredID);

  CollHeap* getHeap()
  {
    return heap_;
  }

  MatchRangePredicates& getRangeMatching()
  {
    return rangeMatching_;
  }

  MatchResidualPredicates& getResidualMatching()
  {
    return residualMatching_;
  }

private:
  // Copy construction/assignment not defined.
  MVCandidate(const MVCandidate&);
  MVCandidate& operator=(const MVCandidate&);

private:
  CollHeap*			heap_;
  const MVDetailsPtr		mvDetails_;
  const DescriptorDetailsPtr	queryDetails_;
  const QRJBBPtr		queryJbb_;
  JBBDetailsPtr                 queryJbbDetails_;
  MVCandidatesForJBBSubsetPtr	jbbSubset_;
  QRJoinSubGraphMapPtr		mvSubGraphMap_;
  NABoolean			isPreferredMatch_;
  GroupingList*			extraGroupingColumns_;
  MatchOutput			outputMatching_;
  MatchRangePredicates		rangeMatching_;
  MatchResidualPredicates	residualMatching_;
  MatchGroupingColumns		groupingMatching_;
  MatchJoinPreds                joinPredMatching_;
  MatchOuterJoins               lojMatching_;
  AggregateMatchingType		matchingType_;
  IDSet                         extraHubTables_;  // Query descriptor IDs of needed extra-hub tables.
  IDSet                         backJoinTables_;  // Query descriptor IDs of needed back-join tables.
  NABoolean			wasDisqualified_; // Avoid dupicate "disqualified" messages.
  TableIDHash                   tableIDCache_;
  NAString*                     disqualifiedReason_;
  static const NAString         trueString_;
  static const NAString         falseString_;
};  // class MVCandidate

/**
 * The MVPair class is used for workload analysis.
 * Workload analysis needs the predicate matching methods of the MVCandidate
 * class, but instead of matching 1 MV to 1 query, here we need to match
 * a group of queries represented by MVDetails objects. The idea is to 
 * match the first query against all the other ones, and use the MVPair class
 * for each such pair. In MVCandidate, the query is "asking the questions"
 * and the MV is "answering" them (questions like: do you have this predicate?)
 * Here we need the first MV to ask the questions and the other MV to answer,
 * so the first MV is playing the query role, and the other MV is playing the 
 * MV role.
 *****************************************************************************
 */
class MVPair : public MVCandidate
{
public:

  MVPair(const QRJoinSubGraphMapPtr  firstMV, 
         const QRJoinSubGraphMapPtr  otherMV,
	 ADD_MEMCHECK_ARGS_DECL(CollHeap* heap));

  virtual ~MVPair() 
  {
  }

  // Find common columns with range and/or residual predicates
  void intersectColumnBitmapsForTable(const NAString& queryTableID, 
                                      XMLBitmap&      rangeBitmap, 
                                      XMLBitmap&      residualBitmap);

  // Does the MV(otherMV) have a matching range predicate?
  NABoolean checkRangePredicate(QRRangePredPtr querySidePred);

  // Does the MV(otherMV) have a matching residual predicate?
  NABoolean checkResidualPredicate(QRExprPtr querySidePred);

  // Is this table an LOJ child?
  NABoolean checkForLOJ(QRTablePtr queryTable);

  NAPtrList<QRRangePredPtr>& getRemainingRangePreds()
  {
    return rangePredList_;
  }

  NAPtrList<QRExprPtr>& getRemainingResidualPreds()
  {
    return residualPredList_;
  }

  virtual QRJoinSubGraphMapPtr getMvSubGraphMap()
  {
    return mvSubGraphMap_;
  }

  virtual QRJoinSubGraphMapPtr getQuerySubGraphMap()
  {
    return querySubGraphMap_;
  }

private:
  QRJoinSubGraphMapPtr        mvSubGraphMap_;
  QRJoinSubGraphMapPtr        querySubGraphMap_;
  NAPtrList<QRRangePredPtr>   rangePredList_;
  NAPtrList<QRExprPtr>        residualPredList_;
};

/**
 * Contains a list of MVCandidate objects, that correspond to the same JBBSubset.
 * Also holds a pointer to the join subgraph, that defines this JBBSubset.
 *****************************************************************************
 */
class MVCandidatesForJBBSubset : public NAIntrusiveSharedPtrObject
{
public:
  /**
   * MVCandidatesForJBBSubset constructor.
   * @param jbbCandidates Parent object: the JBB holding this JBBSubset.
   * @param heap Heap from which to allocate memory.
   */
  MVCandidatesForJBBSubset(MVCandidatesForJBBPtr    jbbCandidates,
			   ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
     ,jbbCandidates_(jbbCandidates)
     ,candidateList_(heap)
     ,hasGroupBy_(FALSE)
     ,querySubGraphMap_(NULL)
     ,minimizedGroupingList_(NULL)
     ,isLoopingOnCandidates_(FALSE)
  { }

  virtual ~MVCandidatesForJBBSubset();

  NABoolean operator==(const MVCandidatesForJBBSubset& other) const;

  /**
   * Insert an MV candidate object into this JBBSubset.
   * @param mv The MVCandidate pointer.
   */
  void insert(MVCandidatePtr mv);

  /**
   * Create an MVCandidate object and insert it into the JBBSubset.
   * @param mv The MV details.
   * @param queryJbb The query details.
   * @param isPreferredMatch Is this a preferred match.
   * @param extraGroupingColumns The extra grouping columns.
   * @param heap Heap from which to allocate memory.
   */
  void insert(MVDetailsPtr	    mv, 
	      QRJBBPtr		    queryJbb,
	      NABoolean		    isPreferredMatch, 
	      GroupingList*	    extraGroupingColumns,
	      QRJoinSubGraphMapPtr  mvMap,
	      CollHeap*		    heap);

  void remove(MVCandidatePtr candidate)
  {
    candidateList_.remove(candidate);
  }

  /**
   * The subGraph is common to all the MVCandidates of this JBBSubset.
   * @param subGraph The subGraph from the MVMemo join graph representing this JBBSubset.
   */
  void setSubGraphMap(QRJoinSubGraphMapPtr querySubGraphMap);

  const QRJoinSubGraphMapPtr getQuerySubGraphMap() const
  {
    return querySubGraphMap_;
  }

  /**
   * Get the MVMemo join graph subGraph for this JBBSubset.
   * @return The MVMemo join graph subGraph for this JBBSubset.
   */
  QRJoinSubGraphPtr getSubGraph() const;

  const NAString* getKeyObject() const;

  /**
   * The the list of MV Candidates for the JBB.
   * @return 
   */
  MVCandidatesForJBBPtr	getJbbCandidates()
  {
    return jbbCandidates_;
  }

  /**
   * Set if this JBBSubset includes a GroupBy.
   * @param groupBy TRUE if this JBBSubset includes a GroupBy.
   */
  void setGroupBy(NABoolean groupBy) { hasGroupBy_ = groupBy; }

  /**
   * Generate the JBBSubset element of the result descriptor including the 
   * rewrite instructions and all the MV candidates inside it.
   * @param heap Heap from which to allocate memory.
   * @return QRJBBSubset object of the result descriptor.
   */
  QRJbbSubsetPtr generateDescriptor(CollHeap* heap);

  /**
   * Get the number of MVCandidate objects contained in this JBBSubset. 
   * @return The number of MVCandidate objects contained in this JBBSubset. 
   */
  ULng32 entries() { return candidateList_.entries(); }


  /**
   * Get MVCandidate object number i from the list.
   * @param i index into the MVCandidate list.
   * @return MVCandidate object number i from the list.
   */
  MVCandidatePtr operator[](Int32 i) { return candidateList_[i]; }

  /**
   * Remove the MVCandidate from the list, because it has been disqualified.
   * @param candidate MV candidate to remove.
   */
  void disqualify(MVCandidatePtr candidate);

  /**
   * Add this MVCandidate into the master list of all the MVCandiates for this query.
   * @param candidate The new MVCandidate to add to the list.
   */
  void registerCandidate(MVCandidatePtr candidate);

  void setMinimizedGroupingList(ElementPtrList* minimizedGroupingList)
  {
    minimizedGroupingList_ = minimizedGroupingList;
  }

  ElementPtrList* getMinimizedGroupingList()
  {
    return minimizedGroupingList_;
  }

  NABoolean isIndirectGroupBy()
  {
    return minimizedGroupingList_ != NULL;;
  }

private:
  MVCandidatesForJBBPtr	    jbbCandidates_;
  NAPtrList<MVCandidatePtr> candidateList_;
  QRJoinSubGraphMapPtr	    querySubGraphMap_;
  NABoolean		    hasGroupBy_;
  ElementPtrList*           minimizedGroupingList_;
  NABoolean                 isLoopingOnCandidates_;
};  // class MVCandidatesForJBBSubset

/**
 * Holds a collection of MVCandidatesForJBBSubset objects with MVCandidates
 *****************************************************************************
 */
class MVCandidatesForJBB : public NAIntrusiveSharedPtrObject
{
public:
  /**
   * MVCandidatesForJBB constructor.
   * @param queryJbb The QRJBB element from the query descriptor.
   * @param allCandidates The parent object - the collection of all the 
   *                      candidates for this query.
   * @param heap Heap from which to allocate memory.
   */
  MVCandidatesForJBB(QRJBBPtr			queryJbb, 
		     MVCandidateCollectionPtr	allCandidates,
		     ADD_MEMCHECK_ARGS_DECL(CollHeap* heap));

  virtual ~MVCandidatesForJBB();

  /**
   * Get the list of all MV candidates for this query.
   * @return The list of all MV candidates for this query.
   */
  MVCandidateCollectionPtr getAllCandidates() 
  { 
    return allCandidates_; 
  }

  NABoolean contains(QRJoinSubGraphPtr subGraph) const;

  /**
   * Insert a JBBSubset into this JBB.
   * @param jbbSubset The JBBSubset object with MVCandidate objects in it.
   */
  void insert(MVCandidatesForJBBSubsetPtr jbbSubset);

  MVCandidatesForJBBSubsetPtr getJbbSubsetFor(QRJoinSubGraphPtr subGraph) const;

  /**
   * Generate the result descriptor for this JBB.
   * @param heap Heap from which to allocate memory.
   * @return The QRJbbResult object for the result descriptor.
   */
  QRJbbResultPtr generateDescriptor(CollHeap* heap);

  /**
   * Add this MVCandidate into the master list of all the MVCandiates for this query.
   * @param candidate The new MVCandidate to add to the list.
   */
  void registerCandidate(MVCandidatePtr candidate);

  /**
   * Add a text line for a QRInfo element in the result descriptor.
   * This is used for Reasons why an MV was disqualified.
   * @param info 
   */
  void addMatchInfo(const NAString* info);

  QRJBBPtr getQueryJbb()
  {
    return queryJbb_;
  }

private:
  MVCandidateCollectionPtr		 allCandidates_;
  JbbSubsetList				 jbbSubsets_;
  QRJBBPtr				 queryJbb_;
  NAList<const NAString *>		 infoList_;
};  // class MVCandidatesForJBB

/**
 * Holds a collection of MVCandidatesForJBB objects with MVCandidates
 *****************************************************************************
 */
class MVCandidateCollection : public NAIntrusiveSharedPtrObject
{
public:
  /**
   * MVCandidateCollection constructor.
   * @param queryDetails The query details
   * @param heap Heap from which to allocate memory.
   */
  MVCandidateCollection(DescriptorDetailsPtr  queryDetails,
			ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
     ,queryDetails_(queryDetails)
     ,jbbs_(heap)
     ,allCandidates_(heap)
  { }

  virtual ~MVCandidateCollection();

  /**
   * Get the query details.
   * @return the query details.
   */
  DescriptorDetailsPtr getQueryDetails() 
  { 
    return queryDetails_; 
  }

  /**
   * Insert a new JBB into the list of JBBs.
   * @param jbb The JBB object to insert.
   */
  void insert(MVCandidatesForJBBPtr jbb) 
  { 
    jbbs_.insert(jbb); 
  }

  /**
   * Generate the result descriptor for this query.
   * @param heap Heap from which to allocate memory.
   * @return QRResultDescriptor object, ready for serialization to XML.
   */
  QRResultDescriptorPtr generateResultDescriptor(CollHeap* heap);

  /**
   * Generate an empty result descriptor.
   * @param heap Heap from which to allocate memory.
   * @return QRResultDescriptor object, ready for serialization to XML.
   */
  static QRResultDescriptorPtr generateEmptyResultDescriptor(CollHeap* heap);

  /**
   * Add this MVCandidate into the master list of all the MVCandiates for this query.
   * @param candidate The new MVCandidate to add to the list.
   */
  void registerCandidate(MVCandidatePtr candidate);

  void disqualifyCandidate(MVCandidatePtr candidate, Int32 index);

  /**
   * Run the matching algorithms on all the MVCandidate objects.
   * This is a temporary method, until the MV candidate reduction algorithm 
   * is implemented in a separate method.
   */
  void doMatching();

private:
  NAPtrList<MVCandidatesForJBBPtr>  jbbs_;
  NAPtrList<MVCandidatePtr>	    allCandidates_;
  DescriptorDetailsPtr		    queryDetails_;
};  // class MVCandidateCollection



#endif
