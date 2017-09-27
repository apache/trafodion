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
#ifndef APPLIEDSTATMAN_HDR
#define APPLIEDSTATMAN_HDR
/* -*-C++-*-
**************************************************************************
*
* File:         AppliedStatMan.h
* Description:  This file includes the class definition for
*               Applied Statistics Manager (ASM).
* Created:      May 6, 2002
* Language:     C++
*
*
*
*
**************************************************************************
*/

// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
//  Include Files
// -----------------------------------------------------------------------

#include "EstLogProp.h"
#include "Analyzer.h"
#include "RelJoin.h"

// -----------------------------------------------------------------------
//  The following classes are defined in this file.
// -----------------------------------------------------------------------

class AppliedStatMan;
class JoinReducer;
class ReducerStat;

// -----------------------------------------------------------------------

// This is the interface/manager class. It contains all methods that
// will be used to interface with ASM.

// Method to initialize ASM is the part of the QueryAnalysis
// class in Analyzer.h

class AppliedStatMan : public NABasicObject
{

public:
  AppliedStatMan(CollHeap *outHeap = CmpCommon::statementHeap());

  ~AppliedStatMan() {};

  // Hash method for ASM cache

  static ULng32 hashASM(const CANodeIdSet & key);

  // traverse through all JBBCs of the given JBB, and compute local
  // predicates, two way joins and set of reducers. It is called from
  // InitializeASM method of QueryAnalysis

  void setupASMCacheForJBB(JBB & jbb);

  // getStatsForCANodeIdSet is similar to getStatsForJBBSubset
  // except that it expects CANodeIdSet as the input

  EstLogPropSharedPtr getStatsForCANodeIdSet(const CANodeIdSet & caNodeSet);

  // getStatsForCANodeId applies local predicates to a JBBC. It is
  // not necessary for the JBBC to be a table.

  EstLogPropSharedPtr getStatsForCANodeId(CANodeId jbbc,
		      const EstLogPropSharedPtr &inLP = (*GLOBAL_EMPTY_INPUT_LOGPROP),
		      const ValueIdSet * predIdSet = NULL);

  // get Stats after applying local predicates to Cluetering key columns of JBBC
  EstLogPropSharedPtr getStatsForLocalPredsOnCKPOfJBBC(CANodeId jbbc,
			const EstLogPropSharedPtr &inLP = (*GLOBAL_EMPTY_INPUT_LOGPROP));

  // get Stats after applying local predicates on the given columns of JBBC
  EstLogPropSharedPtr getStatsForLocalPredsOnPrefixOfColList(CANodeId jbbc,
			const ValueIdList colIdList,
			const EstLogPropSharedPtr &inLP = (*GLOBAL_EMPTY_INPUT_LOGPROP));

  // get Stats after applying local predicates on the given columns of JBBC
  EstLogPropSharedPtr getStatsForLocalPredsOnGivenCols(CANodeId jbbc,
			const ValueIdSet colIdSet,
			const EstLogPropSharedPtr &inLP = (*GLOBAL_EMPTY_INPUT_LOGPROP));

  // get Stats after doing a join on the Clustering key columns of JBBC
  EstLogPropSharedPtr getStatsForJoinPredsOnCKOfJBBC(const CANodeIdSet jbbSubset,
			CANodeId jbbc,
			EstLogPropSharedPtr &inLP = (*GLOBAL_EMPTY_INPUT_LOGPROP));

  // get Stats after applying join predicates on the given columns of JBBC
  EstLogPropSharedPtr getStatsForJoinPredsOnCols(const CANodeIdSet leftChild,
		      CANodeId rightChild,
		      const ValueIdList keyColList,
		      NABoolean OnlyLeadingCols = FALSE,
		      EstLogPropSharedPtr &inLP = (*GLOBAL_EMPTY_INPUT_LOGPROP));

  // get Stats after applying given join predicates on the JBBC
  EstLogPropSharedPtr getStatsForGivenJoinPredsOnJBBC(const CANodeIdSet jbbSubset,
			CANodeId jbbc,
			const ValueIdSet predIdSet,
			const ValueIdSet localPredSet,
			EstLogPropSharedPtr &inLP = (*GLOBAL_EMPTY_INPUT_LOGPROP));

  // getStatsForJBBSubset can be used to get cached statistics for the
  // given JBBSubset. if the statistics is not cached, it will compute
  // it incrementally. This method should be used when you have already
  // computed statistics for the same JBBSubset before. If not, you
  // should use JoinJBBSubsets (or any other join methods)for
  // computing statistics.

  EstLogPropSharedPtr getStatsForJBBSubset(const JBBSubset & jbbSubset);

  // joinJBBChildren is used to join two CANodeIdSets. This is
  // less expensive than joinJBBSubsets but the user should be
  // careful while sending in leftNodeSets and rightNodeSets. Both
  // these CANodeIdSets should correspond to the JBBCs from the
  // same JBB.

  EstLogPropSharedPtr joinJBBChildren(const CANodeIdSet & leftNodeSet,
			      const CANodeIdSet & rightNodeSet,
			      EstLogPropSharedPtr &inLP = (*GLOBAL_EMPTY_INPUT_LOGPROP));

  // do a fast computation of the join reduction based only on the
  // jbbcs that are involved in the join between the two sets

  CostScalar computeJoinReduction(const CANodeIdSet & leftNodeSet,
                                  const CANodeIdSet & rightNodeSet);

  // get cached estimated logical proeprties from the ASM. If the
  // properties are not cached, return NULL.

  EstLogPropSharedPtr getCachedStatistics( const CANodeIdSet * combinedNodeSet);

  // add pointer to estLogProps in the ASM cache.
  NABoolean insertCachePredStatEntry (const CANodeIdSet & jbbNodeSet,
				      const EstLogPropSharedPtr& estLogProp);

  // lookup is used to see if the estimated logical properties
  // for the given CANodeIdSet exist in the ASM cache.

  NABoolean lookup (const CANodeIdSet &key1) const;

  // removeEntryIfThisObjectIsCached is used, when the corresponding estLogProp
  // is deleted from groupAttributes. This method is called from the destructor
  // of EstLogProp. This is to avoid the case of hanging pointers in ASM cache.

  void removeEntryIfThisObjectIsCached(EstLogProp * lp);

private:

  // Get expression for CANodeId
  RelExpr * getExprForCANodeId(
            CANodeId jbbc,
            const EstLogPropSharedPtr &inLP = (*GLOBAL_EMPTY_INPUT_LOGPROP),
            const ValueIdSet * predIdSet =  NULL);
            
  // joinEstLogProps can be used to join the estimated logical
  // prperties of the left and the right children.

  EstLogPropSharedPtr joinEstLogProps (const EstLogPropSharedPtr& leftEstLogProp,
  			     const EstLogPropSharedPtr& rightEstLogProp,
  			     const EstLogPropSharedPtr& inputEstLogProp =\
   			               (*GLOBAL_EMPTY_INPUT_LOGPROP));

  // form join expressions for the given left and right children

  Join * formJoinExprWithCANodeSets(const CANodeIdSet & leftChild,
				    const CANodeIdSet & rightChild,
				    EstLogPropSharedPtr& inLP,
				    const ValueIdSet * joinPreds,
				    const NABoolean cacheable);

  Join * formJoinExprWithEstLogProps(const EstLogPropSharedPtr& leftEstLogProp,
				 const EstLogPropSharedPtr& rightEstLogProp,
				 const EstLogPropSharedPtr& inputEstLogProp,
				 const ValueIdSet * setOfPredicates,
				 const NABoolean cacheable,
				 JBBSubset * combinedJBBSubset = NULL);

  // This method forms the join expression for join on JBBC specified by jbbcId
  // inputEstLogProp should not be cacheable
  Join * formJoinExprForJoinOnJBBC(
         CANodeIdSet jbbSubset,
         CANodeId    jbbcId,
         const ValueIdSet * jbbcLocalPreds,
         const ValueIdSet * joinPreds,
         const EstLogPropSharedPtr& inputEstLogProp,
         const NABoolean cacheable);

  // This is needed to set the potential outputs for the JBBSubset
  // while faking the joinExpr

  ValueIdSet getPotentialOutputs(const CANodeIdSet & nodeSet);

  // synthesize logical properties for the given JBBSubset.
  // This will be used if the properties for the given JBBSubset
  // do not exist in the ASM cache

  EstLogPropSharedPtr synthesizeLogProp(
		    const CANodeIdSet * nodeSet,
		    EstLogPropSharedPtr &inLP = (*GLOBAL_EMPTY_INPUT_LOGPROP));

  NAHashDictionary <CANodeIdSet, EstLogProp> * cacheASM_;

}; // AppliedStatMan
#endif // APPLIEDSTATMAN_H
