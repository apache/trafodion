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
* File:         PhyProp.C
* Description:  Physical Properties and Cost
* Created:      8/28/94
* Language:     C++
*
*
*
*
******************************************************************************
*/

// -----------------------------------------------------------------------

#include "Sqlcomp.h"
#include "CmpContext.h"
#include "GroupAttr.h"
#include "PhyProp.h"
#include "Cost.h"
#include "SearchKey.h"
#include "ItemOther.h"
#include "Globals.h"


// -----------------------------------------------------------------------
// methods for PhysicalProperty
// -----------------------------------------------------------------------

PhysicalProperty::~PhysicalProperty() {}

void PhysicalProperty::print(FILE* ofd,
			     const char * /* prefix */,
			     const char * /* suffix */) const
{
}

// Checked during SQ M5. This function is not used at all. 
// Determines if the partitioning key is a prefix of the clustering key.
NABoolean PhysicalProperty::isPartKeyPrefixOfSortKey() const
{
  const PartitioningFunction* partFunc = getPartitioningFunction();

  // if there is no partitioning key then this is OK
  if (partFunc == NULL)
    return TRUE;

  // The test makes no sense unless the partitioning function is a range
  // partitioning function.
  if (NOT partFunc->isARangePartitioningFunction())
    return FALSE;

  ValueIdList actualPartKey(
    partFunc->castToRangePartitioningFunction()->getKeyColumnList());
  ValueIdList actualSortKey(getSortKey());
  Lng32 numPartCols = actualPartKey.entries();
  Lng32 numClusterCols = actualSortKey.entries();

  for (CollIndex index = 0; index < CollIndex(numPartCols); index++)
  {
    if (actualPartKey[index] != actualSortKey[index])
      return FALSE;
  }

  return TRUE;

} // PhysicalProperty::isPartKeyPrefixOfSortKey()

// The following method is used by the overloaded implementations of
// '<', '>', '==' for physical properties.
COMPARE_RESULT
PhysicalProperty::compareProperties (const PhysicalProperty &other) const
{
  // ---------------------------------------------------------------------
  // Identity test.
  // ---------------------------------------------------------------------
  if (this == &other)  // are they identical ?
    return SAME;

  // ---------------------------------------------------------------------
  // return
  //
  // - SAME, if both objects describe the same physical properties
  // - MORE, if this object describes more restrictive physical properties
  //         than the other (e.g. this describes sorting by a,b, other
  //         describes sorting by a)
  // - LESS, if this object describes less restrictive physical properties
  // - INCOMPATIBLE, if this object describes physical properties that can
  //         not be fulfilled at the same time as the other physical props,
  //         (e.g. 'this' is sorted by a, 'other' is sorted by b)
  // - UNDEFINED, if the result is undefined, e.g. 'this' is sorted by
  //         a,b and not partitioned, and 'other' is sorted by a and
  //         partitioned by a.
  // ---------------------------------------------------------------------
  COMPARE_RESULT result = SAME;

  // ---------------------------------------------------------------------
  // Compare the sort key expressions.
  // The two sort keys MUST have a common prefix, i.e., identical
  // expressions (same ValueId, same NAType, etc.), consisting of
  // the shorter of the two keys.
  // ---------------------------------------------------------------------
  Lng32 myKeyCount = sortKey_.entries();
  Lng32 otherKeyCount = other.sortKey_.entries();
  Lng32 minEntries = MINOF(myKeyCount,otherKeyCount);

  for (CollIndex index = 0; index < (CollIndex)minEntries; index++)
    if (NOT (sortKey_[index] == other.sortKey_[index]))
      {
	// two different sorts required, can't do both of them at the same time
	return INCOMPATIBLE;
      }

	if(explodedOcbJoin_ != other.explodedOcbJoin_)
    return INCOMPATIBLE;

  if (myKeyCount > otherKeyCount)
    // i dominate other if i my sort key contains more columns
    result = combine_compare_results(result,MORE);
  else if (myKeyCount < otherKeyCount)
    // other dominates me if its sort key contains more columns
    result = combine_compare_results(result,LESS);
  // else both are sorted (or not sorted) by the same columns, result == SAME

  // ---------------------------------------------------------------------
  // Compare Sort Order Type
  // ---------------------------------------------------------------------
  if (sortOrderType_ != other.sortOrderType_)
  {
    if (sortOrderType_ == NO_SOT)
      result = combine_compare_results(result,LESS);
    else if (other.sortOrderType_ == NO_SOT)
      result = combine_compare_results(result,MORE);
    else
      return INCOMPATIBLE;
  }

  // ---------------------------------------------------------------------
  // Compare the DP2 Sort Order Partitioning Function
  // ---------------------------------------------------------------------
  if ((dp2SortOrderPartFunc_ != NULL) AND
      (other.dp2SortOrderPartFunc_ != NULL))
    result = combine_compare_results
               (result,
	        dp2SortOrderPartFunc_->comparePartFuncToFunc
		   (*(other.dp2SortOrderPartFunc_)));
  else if (dp2SortOrderPartFunc_ != NULL)
    result = combine_compare_results(result,MORE);
  else if (other.dp2SortOrderPartFunc_ != NULL)
    result = combine_compare_results(result,LESS);

  // ---------------------------------------------------------------------
  // Compare the locations contained in each property.
  // ---------------------------------------------------------------------
  if (location_ != other.location_)
    {
      // location DP2 is incompatible with any other location
      if (location_ == EXECUTE_IN_DP2 OR
	  other.location_ == EXECUTE_IN_DP2)
	return INCOMPATIBLE;

      // actual properties should never contain wildcards
      CMPASSERT(location_ != EXECUTE_IN_MASTER_OR_ESP AND
		other.location_ != EXECUTE_IN_MASTER_OR_ESP);

      // Both locations are outside of DP2, now take care of the
      // subtleties of ESP locations. A plan that can execute in
      // both the master and in an ESP provides more than one that
      // can't.
      if (location_ == EXECUTE_IN_MASTER_AND_ESP)
	result = combine_compare_results(result,MORE);
      else if (other.location_ == EXECUTE_IN_MASTER_AND_ESP)
	result = combine_compare_results(result,LESS);
      else
	// A plan that executes in the ESP only is incompatible with
	// one that executes in the master only.
	return INCOMPATIBLE;
    }

  // ---------------------------------------------------------------------
  // Compare the partitioning criteria.
  // ---------------------------------------------------------------------
  if (isPartitioned() AND other.isPartitioned())
    result = combine_compare_results
               (result,
	        getPartitioningFunction()->comparePartFuncToFunc
		   (*(other.getPartitioningFunction())));
  // I am partitioned but the other is not, I am more dominant.
  else if (isPartitioned())
    result = combine_compare_results(result,MORE);
  // The other is partitioned but I am not, I am less dominant.
  else if (other.isPartitioned())
    result = combine_compare_results(result,LESS);
  // Neither of us are partitioned. We are SAME.


  return result;

} // PhysicalProperty::compareProperties

// The following method currently removes anything in the sort key that's
// not covered by the group attributes.  Someday we may expand it to cover
// the partitioning function.
//
// NB: There is similar to what's in FileScan::synthPhysicalProperty(),
// but not identical.  This is because FileScan doesn't have to worry
// about complex sort orders (or even multi-column sort orders) that look
// like, e.g., A+B, CAST (A as ...), ....

void
PhysicalProperty::enforceCoverageByGroupAttributes (const GroupAttributes * groupAttr)
{
  ValueIdList newSortKey = getSortKey() ;
  ValueIdSet coveringSet = groupAttr->getCharacteristicInputs();

  // This first function call might not do anything, since constants should
  // have been filtered out at the leaf.
  newSortKey.removeCoveredExprs(coveringSet);

  // This was added to fix the problem exposed by the case 10-010321-1842
  // Compiler failed to create a plan when query had sort order req. by
  // column number which is expression containing dynamic parameter and
  // covered by another column in RelRoot requiredOutput like
  // SELECT a,a/(?p) FROM t ORDER BY 2; SortKey produced by Sort
  // operator got removed by this function because characteristic
  // output had only "a", expression got removed in Normilizer as covered
  // expression (see comment about this case in NormRelExpr.cpp). Previously
  // complifyAndRemoveUncoveredSuffix() had only characteristic output
  // in the argument. Now characteristic input is also added.
  coveringSet += groupAttr->getCharacteristicOutputs();
  newSortKey.complifyAndRemoveUncoveredSuffix(coveringSet, groupAttr);

  if (newSortKey.isEmpty())
  {
    sortOrderType_ = NO_SOT;
    dp2SortOrderPartFunc_ = NULL;
  }

  sortKey_ = newSortKey ; // might not have changed
} // PhysicalProperty::enforceCoverageByGroupAttributes

// -----------------------------------------------------------------------
// methods for PerformanceGoal
// -----------------------------------------------------------------------

PerformanceGoal::~PerformanceGoal() {}

NABoolean PerformanceGoal::isOptimizeForFirstRow() const   { return FALSE; }

NABoolean PerformanceGoal::isOptimizeForLastRow() const    { return FALSE; }

NABoolean PerformanceGoal::isOptimizeForResourceConsumption() const
                                                         { return FALSE; }

// -- Optimize for N rows
NABoolean OptimizeForNRows::isOptimizeForFirstRow() const   { return TRUE; }

// -- Optimize for last row
NABoolean OptimizeForLastRow::isOptimizeForLastRow() const  { return TRUE; }
NABoolean OptimizeForLastRow::isOptimizeForFirstRow() const   { return FALSE; }

// -- Optimize for resource consumption
NABoolean
OptimizeForResourceConsumption::isOptimizeForResourceConsumption() const
                                                         { return TRUE; }

// -----------------------------------------------------------------------
// methods for ReqdPhysicalProperty
// -----------------------------------------------------------------------

ReqdPhysicalProperty::~ReqdPhysicalProperty()
{
}


void ReqdPhysicalProperty::print(FILE* ofd,
				 const char * /* prefix */,
				 const char * /* suffix */) const
{
  fprintf(ofd,"Available CPUs: %d\n Pipelines per CPU: %d\n",
 availableCPUs_, pipelinesPerCPU_);
}


// --------------------------------------------------------------------
// does a given plan satisfy the required property
// ---------------------------------------------------------------------
NABoolean ReqdPhysicalProperty::satisfied (
            EstLogPropSharedPtr inputLogProp,
            const RelExpr * const physExpr,
	    const PhysicalProperty * const physProp) const
{

  // ----------------------------------------------------------------------
  // If physical properties have not yet been synthesized, we can't assume
  // that they satisfy the requirement, so return FALSE.
  // ----------------------------------------------------------------------
  if (physProp == NULL)
    {
      return FALSE;
    }

  GroupAttributes *ga = physExpr->getGroupAttr();

  // ---------------------------------------------------------------------
  // check requirement for arrangement of data
  // ---------------------------------------------------------------------
  if (arrangedBy_ != NULL AND
      NOT physProp->getSortKey().satisfiesReqdArrangement(*arrangedBy_,ga))
    return FALSE;

  // ---------------------------------------------------------------------
  // check requirements for ordering
  // ---------------------------------------------------------------------
  if (orderedBy_ != NULL AND
      (physProp->getSortKey().satisfiesReqdOrder(*orderedBy_,ga) != SAME_ORDER))
    return FALSE;

  // ---------------------------------------------------------------------
  // Check Sort Order Type Requirement
  // ---------------------------------------------------------------------
  if (NOT sortOrderTypeReqAndSynthCompatible(physProp->getSortOrderType()))
    return FALSE;

  // ---------------------------------------------------------------------
  // Check any DP2 Sort Order Partitioning Requirement
  // ---------------------------------------------------------------------
  if (dp2SortOrderPartReq_ != NULL)
  {
    if (NOT dp2SortOrderPartReqAndSynthCompatible(physProp))
      return FALSE;
  }

  // ---------------------------------------------------------------------
  // check requirements for where the plan should execute
  // ---------------------------------------------------------------------
  // If the actual location does not match the requirement
  // exactly, then we still have a couple of ways to succeed:
  // - the requirement is for EXECUTE_IN_MASTER_OR_ESP and
  //   the actual plan executes outside of DP2,
  // - the requirement asks for something outside of DP2
  //   (master, ESP, either, or both) and the actual plan
  //   can execute in both master and ESP
  if (location_ != physProp->getPlanExecutionLocation() AND
       NOT ((location_ == EXECUTE_IN_MASTER_OR_ESP AND
	     NOT physProp->executeInDP2()) OR
	   (location_ != EXECUTE_IN_DP2 AND
	    physProp->getPlanExecutionLocation() ==
	    EXECUTE_IN_MASTER_AND_ESP)))
    {
      // If we reach here we failed.
      return FALSE;
    }

  // ---------------------------------------------------------------------
  // check requirements for partitioning
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // If I asked for a specific partitioning function, then compare the
  // required partitioning function against the actual partitioning
  // function.
  // ---------------------------------------------------------------------
  if (requiresPartitioning())
  {
      if ( NOT getPartitioningRequirement()->partReqAndFuncCompatible
           (physProp->getPartitioningFunction()))
        return FALSE;

      // No need to check skewness specifically here. It has been already 
      // checked inside partReqAndFuncCompatible() above in
      // (myPartFunc->comparePartFuncToFunc(*other)). 
  }


  // Push-down requirement check
  const PushDownRequirement* pdr = getPushDownRequirement();
  if ( pdr && pdr->isEmpty() == FALSE )
  {
      const PushDownProperty* pdp = physProp->getPushDownProperty();

      if ( pdp == NULL OR pdp->isEmpty() == TRUE OR
           NOT(pdr->satisfied(*pdp))
         )
        return FALSE;
  }

  // ---------------------------------------------------------------------
  // check logical partitioning requirement
  // ---------------------------------------------------------------------
  if (logicalPartReq_ AND NOT logicalPartReq_->satisfied(physExpr,physProp))
    return FALSE;

  // ---------------------------------------------------------------------
  // check requirements for synchronous/asynchronous access
  // ---------------------------------------------------------------------

  // We must ensure that if either the user forced synchronous access,
  // or if there is a logical order or arrangement requirement that
  // will need synchronous access to satisfy it, that our partitioning
  // function actually exhibits synchronous access. Conversely,
  // if the user forced asynchronous access, or synchronous access
  // is not possible or not necessary, then we need to ensure that
  // our partitioning function does not exhibit synchronous access.

  // Synchronous/asynchronous only applies to operators in DP2,
  // so we only need to worry if the partitioning function is
  // either a logPhysPartFunc or a single partition partitioning
  // function.
  const LogPhysPartitioningFunction* logPhysPartFunc =
    physProp->getPartitioningFunction()->
      castToLogPhysPartitioningFunction();

  // If the part func was a single partition partitioning function,
  // then synchronous access is not necessary and cannot occur.
  // For that matter, asynchronous access is not necessary and
  // cannot occur. So, there is nothing to worry about in this case.

  // If the partitioning function was a LogPhysPartFunc, then
  // using the physical partitioning function of the spp, we see
  // if synchronous access is what is needed to satisfy any logical
  // order and/or arrangement or force requirements, and that
  // synchronous access is possible. If so, then the spp had better
  // show that synchronous access will occur. If not, then the spp
  // had better show that synchronous access will NOT occur. Otherwise,
  // we cannot use this solution. Note that the "trick" that is
  // occuring here is that we are calling shouldUseSynchronousAccess
  // here with the current rpp ("this"), whereas the
  // shouldUseSynchronousAccess flag in the logPhysPartFunc may
  // have been set based on some different rpp. This could
  // occur, for example, if we were attempting to "steal" a
  // solution from some previous context.
  if ((logPhysPartFunc != NULL) AND
      (logPhysPartFunc->
        getPhysPartitioningFunction()->
        shouldUseSynchronousAccess(this,inputLogProp,ga) !=
      logPhysPartFunc->getSynchronousAccess()))
    return FALSE;

  // ---------------------------------------------------------------------
  // check requirement to match a specific pattern (let map value ids,
  // PACK and UNPACK slide through unchecked, since they are basically 
  // no-ops and need not be specified in a CONTROL QUERY SHAPE directive)
  // and some other operators
  // ---------------------------------------------------------------------
  if (mustMatch_ != NULL AND
      NOT physExpr->patternMatch(*mustMatch_) AND
      physExpr->getOperatorType() != REL_MAP_VALUEIDS AND
      physExpr->getOperatorType() != REL_PACK AND
      physExpr->getOperatorType() != REL_UNPACKROWS AND
      physExpr->getOperatorType() != REL_FIRST_N AND
      (NOT CURRSTMT_OPTDEFAULTS->ignoreExchangesInCQS() OR
       physExpr->getOperatorType() != REL_EXCHANGE) AND
      (NOT CURRSTMT_OPTDEFAULTS->ignoreSortsInCQS() OR
       physExpr->getOperatorType() != REL_SORT))
    return FALSE;

  
/*
  // disable parallele execution for NESTED_JOINs that control row triggers
  // execution. Parallel execution for row triggers introduces the
  // potential for non-deterministic execution
  long degreeOfParallelism = physProp->getCountOfPartitions();
  if (((physExpr->getOperatorType() == REL_NESTED_JOIN)  || (physExpr->getOperatorType() == REL_LEFT_NESTED_JOIN)) && 
      (degreeOfParallelism > 1) &&
      (physExpr->getInliningInfo().isSingleExecutionForTriggersTSJ()))
    return FALSE;
*/

  return TRUE; // all checks passed

} // ReqdPhysicalProperty::satisfied ()

COMPARE_RESULT
ReqdPhysicalProperty::compareRequirements(const ReqdPhysicalProperty &other) const
{
  // ---------------------------------------------------------------------
  // Identity test.
  // ---------------------------------------------------------------------
  if (this == &other)  // are they identical ?
    return SAME;

  // ---------------------------------------------------------------------
  // return
  //
  // SAME         if both required props require the same features
  // MORE         if this property dominates the other, e.g., this
  //              requires sorting by a,b, other requires sorting
  //              by a alone
  // LESS         if this property is dominated by the other one
  // INCOMPATIBLE if this property cannot be fulfilled at the same time
  //              as the other property, e.g., this requires sorting by
  //              a, other requires sorting by b
  // UNDEFINED    if the result is undefined, e.g., this requires sorting
  //              by a,b and no partitioning, and other requires sorting
  //              and partitioning by a.
  // ---------------------------------------------------------------------

  COMPARE_RESULT result = SAME; // initialization

  // ---------------------------------------------------------------------
  // compare optimization goals and information on input values
  // ---------------------------------------------------------------------

  // can't use any plan if the optimization goals are different
  // $$$$ may apply some fuzzy logic in the future for the number of rows
  if ( (perfGoal_ != NULL) AND
       (( perfGoal_->isOptimizeForLastRow() AND
         (NOT other.perfGoal_->isOptimizeForLastRow()) ) OR
        ( perfGoal_->isOptimizeForFirstRow() AND
         (NOT other.perfGoal_->isOptimizeForFirstRow()) ) OR
        ( perfGoal_->isOptimizeForResourceConsumption() AND
         (NOT other.perfGoal_->isOptimizeForResourceConsumption()) )) )
    return UNDEFINED;

  // if the cost weights are different, the optimal solution is no
  // longer reliable, but we can still search the context for possible
  // solutions
  if (costWeight_ != other.costWeight_)
    result = combine_compare_results(result,UNDEFINED);

  // ---------------------------------------------------------------------
  // compare the sort order of both properties
  // ---------------------------------------------------------------------
  if (orderedBy_ != NULL OR other.orderedBy_ != NULL)
    {
      Lng32 mySortEntries;
      Lng32 otherSortEntries;

      // Compute the number of sort key columns.

      if (orderedBy_ != NULL)
	mySortEntries = orderedBy_->entries();
      else
	mySortEntries = 0;

      if (other.orderedBy_ != NULL)
	otherSortEntries = other.orderedBy_->entries();
      else
	otherSortEntries = 0;

      // The columns in coreesponding positions must be the same.
      for (CollIndex i = 0; i < (CollIndex)MINOF(mySortEntries,otherSortEntries); i++) 
	{
	  if (NOT (orderedBy_->at(i) == other.orderedBy_->at(i)))
	    {
	      // two different sorts required,
	      // can't do both of them at the same time
	      return INCOMPATIBLE;
	    }
	}

      // i dominate the other if i have more key columns
      if (mySortEntries > otherSortEntries)
	result = combine_compare_results(result,MORE);
      // other domiante me if it have more key columns
      else if (mySortEntries < otherSortEntries)
	result = combine_compare_results(result,LESS);
      // else both are sorted by the same columns,
      // result == SAME
    }

  // ---------------------------------------------------------------------
  // With the partial result in 'result', continue comparing the arranged
  // columns.
  //
  // Now here's a phenomenon: We expect that if a physical
  // property P satisfies a required property R1, then it must also
  // satisfy a less restrictive required property R2 (Lemma 1).
  //
  // Intuitively we would say that requiring more arranged columns is
  // more restrictive. However, that would cause the above Lemma 1 to
  // be violated. What's the solution: consider a smaller set of
  // arranged columns MORE restrictive. To make that easier to swallow,
  // just think about that a smaller set leaves fewer choices to possible
  // sort keys.
  //
  // It still holds that requiring no arrangements is less restrictive
  // than requiring some.
  //
  // UNDEFINED vs. INCOMPATIBLE: two sets of arranged columns that contain
  // distinct elements (neither set is a superset of the other) are
  // INCOMPATIBLE, since no sort key can be found that has the columns
  // from both sets - without intervening columns - as a prefix.
  // ---------------------------------------------------------------------
  if (arrangedBy_ != NULL OR other.arrangedBy_ != NULL)
    {
      // if only one of the reqd props requires arranged columns, then
      // that reqd prop is more restrictive
      if (other.arrangedBy_ == NULL)
	{
	  // only "this" requires arrangements, this usually means that
	  // "this" is more restrictive, except if "other" requires a
	  // sort order that contains the arranged columns of "this"
	  if (other.orderedBy_ != NULL)
	    {
	      if (arrangedBy_->isDensePrefix(*other.orderedBy_))
		result = combine_compare_results(result,LESS);
	      else
		// if my arranged cols don't match the other sort
		// prefix, then there exists no plan that satisfies both
		// reqd props
		return INCOMPATIBLE;
	    }
	  else
	    result = combine_compare_results(result,MORE);
	}
      else if (arrangedBy_ == NULL)
	{
	  if (orderedBy_ != NULL)
	    {
	      if (other.arrangedBy_->isDensePrefix(*orderedBy_))
		result = combine_compare_results(result,MORE);
	      else
		return INCOMPATIBLE;
	    }
	  else
	    // we're less restrictive (only "other" requires arrangements)
	    result = combine_compare_results(result,LESS);
	}
      else
	{
	  // both reqd props require arrangements

	  // do nothing if the sets are equal
	  if (NOT (*arrangedBy_ == *other.arrangedBy_))
	    {
	      if (arrangedBy_->contains(*other.arrangedBy_))
		// we're less restrictive, since we have more columns
		// in the arranged columns set (see phenomenon above)
		result = combine_compare_results(result,LESS);
	      else if (other.arrangedBy_->contains(*arrangedBy_))
		// ditto, just the other way round
		result = combine_compare_results(result,MORE);
	      else
		// sets with distinct elements are incompatible (see above)
		result = combine_compare_results(result,INCOMPATIBLE);
	    }
	}
    }


  // ---------------------------------------------------------------------
  // Compare Sort Order Type Requirements
  // ---------------------------------------------------------------------
  result = combine_compare_results(result,
             compareSortOrderTypeReqToReq(other.sortOrderTypeReq_));

  // ---------------------------------------------------------------------
  // Compare the DP2 Sort Order Partitioning Requirements
  // ---------------------------------------------------------------------
  if ((dp2SortOrderPartReq_ != NULL) AND
      (other.dp2SortOrderPartReq_ != NULL))
    result = combine_compare_results
               (result,
	        dp2SortOrderPartReq_->comparePartReqToReq
		   (other.dp2SortOrderPartReq_));
  else if (dp2SortOrderPartReq_ != NULL)
    result = combine_compare_results(result,MORE);
  else if (other.dp2SortOrderPartReq_ != NULL)
    result = combine_compare_results(result,LESS);

  // ---------------------------------------------------------------------
  // compare the logicalOrderOrArrangement flag of both properties
  // ---------------------------------------------------------------------
  if (logicalOrderOrArrangement_ != other.logicalOrderOrArrangement_)
  {
    // A context which has the logicalOrderOrArrangement_ flag set to
    // TRUE requires MORE than one where it is FALSE. Otherwise,
    // a context where it is set to FALSE is UNDEFINED compared to
    // one where it is TRUE.
    if ((logicalOrderOrArrangement_ == TRUE) AND
        (other.logicalOrderOrArrangement_ == FALSE))
    {
      result = combine_compare_results(result,MORE);
    }
    else
    {
      result = combine_compare_results(result,UNDEFINED);
    }
  }

  //----------------------------------------------------------------------
  // No ESP Exchange requirement:
  // We want to rule out plan stealing if there is difference in ESP Exchange
  // Requirements. We may revisit this issue when we have a property
  // that indicates there is an ESP exchange underneath the current operator
  // We need to add this property.
  //----------------------------------------------------------------------

  if (getNoEspExchangeRequirement() && 
       !other.getNoEspExchangeRequirement()
     )
     result = combine_compare_results(result,INCOMPATIBLE);
  else if (!getNoEspExchangeRequirement() && 
            other.getNoEspExchangeRequirement()
          )
     result = combine_compare_results(result,INCOMPATIBLE);

  // ---------------------------------------------------------------------
  // Compare the locations contained in each property.
  // If the properties don't care about the location for execution or
  // if both of them contain the same location for execution, transmit
  // result unchanged. Otherwise, the two properties contain incompatible
  // locations for execution.
  // ---------------------------------------------------------------------
  if (location_ != other.location_)
    {
      // location DP2 is incompatible with any other location
      if (location_ == EXECUTE_IN_DP2 OR
	  other.location_ == EXECUTE_IN_DP2)
	return INCOMPATIBLE;

      // Both locations are outside of DP2, now take care of the
      // subtleties of ESP locations. If one requirement doesn't
      // care about the difference between master and ESP then it
      // requires less. If one requirement asks for a plan that
      // can execute in both master and ESP then that's asking for
      // more than execution in either location.
      if (location_ == EXECUTE_IN_MASTER_OR_ESP)
	result = combine_compare_results(result,LESS);
      else if (other.location_ == EXECUTE_IN_MASTER_OR_ESP)
	result = combine_compare_results(result,MORE);
      else if (location_ == EXECUTE_IN_MASTER_AND_ESP)
	result = combine_compare_results(result,MORE);
      else if (other.location_ == EXECUTE_IN_MASTER_AND_ESP)
	result = combine_compare_results(result,LESS);
      else
	// A plan that executes in the ESP only is incompatible with
	// one that executes in the master only.
	return INCOMPATIBLE;
    }

  // ---------------------------------------------------------------------
  // Compare the partitioning criteria.
  // ---------------------------------------------------------------------
  // Both are partitioned, compare the partitioning functions
  if (requiresPartitioning() AND other.requiresPartitioning())
    result = combine_compare_results
               (result,
	        getPartitioningRequirement()
                  ->comparePartReqToReq(other.getPartitioningRequirement()));
  // I dominate
  else if (requiresPartitioning())
    result = combine_compare_results(result,MORE);
  // The other dominates
  else if (other.requiresPartitioning())
    result = combine_compare_results(result,LESS);
  // Neither of us require partitioning. We are SAME.

  // ---------------------------------------------------------------------
  // Compare logical partitioning requirements
  // ---------------------------------------------------------------------
  if (logicalPartReq_ AND other.logicalPartReq_)
    {
      result = combine_compare_results(
	   result,
	   logicalPartReq_->compareLogPartRequirements(
		*other.logicalPartReq_));
    }
  else if (logicalPartReq_)
    result = combine_compare_results(result,MORE); // I dominate
  else if (other.logicalPartReq_)
    result = combine_compare_results(result,LESS); // other dominates

  // ---------------------------------------------------------------------
  // Compare the degrees of operator parallelism.
  // If the performance goal is to return the first row and the last row
  // fast, then a plan that uses more more parallelism is prefered
  // over one that does not.
  // If the performance goal is to keep resource consumption under
  // control, then a plan that uses less parallelism is prefered.
  // ---------------------------------------------------------------------
  if (NOT executeInDP2() AND NOT other.executeInDP2())
    {
      Lng32 myPipelines = getCountOfPipelines();
      Lng32 otherPipelines = other.getCountOfPipelines();

      if (perfGoal_->isOptimizeForFirstRow() OR
          perfGoal_->isOptimizeForLastRow())
	{
	  if (myPipelines > otherPipelines)
	    result = combine_compare_results(result,MORE);
	  else if (myPipelines < otherPipelines)
	    result = combine_compare_results(result,LESS);
	  // else the two degrees of parallelism are equal
	}
      else if (perfGoal_->isOptimizeForResourceConsumption())
	{
	  if (myPipelines > otherPipelines)
	    result = combine_compare_results(result,LESS);
	  else if (myPipelines < otherPipelines)
	    result = combine_compare_results(result,MORE);
	  // else the two degrees of parallelism are equal
	}
    }

  // ---------------------------------------------------------------------
  // Compare the pattern that the plan must match.
  // ---------------------------------------------------------------------
  if (mustMatch_ != other.mustMatch_)
    result = combine_compare_results(result,UNDEFINED);


 /* if (mustMatch_ != NULL OR other.mustMatch_ != NULL)
    {
      if (mustMatch_ == NULL)
	result = combine_compare_results(result,LESS);
      else if (other.mustMatch_ == NULL)
	result = combine_compare_results(result,MORE);
      else
	{
	  // give up, if the two required patterns aren't identical
	  if (mustMatch_ != other.mustMatch_)
	    result = combine_compare_results(result,UNDEFINED);
	}
    }*/


  // ---------------------------------------------------------------------
  // compare the push down requiement of both properties
  // ---------------------------------------------------------------------

  if ( pushDownRequirement_ == other.pushDownRequirement_ )
  {
    // same PushDownRequirement object skip;
  }
  else if (pushDownRequirement_ == NULL)
  {
    result = combine_compare_results(result, INCOMPATIBLE);
  }
  else if (other.pushDownRequirement_ == NULL)
  {
    // only one of them is NULL
    result = combine_compare_results(result, INCOMPATIBLE);
  }
  else 
  {
    result = 
      combine_compare_results(result, 
      pushDownRequirement_->compare(*other.pushDownRequirement_));
  }

  return result;

} // ReqdPhysicalProperty::compareRequirements


// Checked in M5. This function is never called
RelExpr * ReqdPhysicalProperty::getInputMustMatch(Lng32 childIndex) const
{
  // If the solution for this context must or need not match
  // a particular pattern, what is the pattern that an input
  // context for the child at index "childIndex" must match?

  if (mustMatch_ == NULL)
    return NULL; // no pattern match required for this and for input
  else
    if (mustMatch_->getArity() > childIndex)
      // input must match i-th pattern input
      return mustMatch_->child((Int32) childIndex);
    else
      return NULL; // pattern has fewer children, no need for a match
}

// ||opt this method should never be called because we always
// have a location requirement???
NABoolean ReqdPhysicalProperty::isEmpty() const
{
  // check whether no ordering or partitioning requirements are there
  // and whether the optimization goals have the default values

  // A physical property is always required (a location is always required)
  //   so always return FALSE.

  // THE FOLLOWING OLD CODE IS COMMENTED OUT 6/18/98 (hl)
  //return (arrangedBy_ == NULL AND
  //	  orderedBy_ == NULL AND
  //	  partReq_ == NULL AND
  //	  location_ == EXECUTE_IN_MASTER AND
  //	  perfGoal_ == DefaultPerformanceGoal AND
  //	  costWeight_ == DefaultCostWeight AND
  //	  mustMatch_ == NULL);

  return FALSE;
}

NABoolean
PushDownColocationRequirement::satisfied(const PushDownProperty& other) const
{
  const PushDownColocationProperty* x =
	other.castToPushDownColocationProperty();

  if ( x == NULL ) return FALSE;

  const NodeMap* thisNodeMap = getNodeMap();
  const NodeMap* otherNodeMap = x->getNodeMap();

  if (thisNodeMap == NULL || otherNodeMap == NULL )
    return FALSE;

  if (thisNodeMap == otherNodeMap)
    return TRUE;

  return thisNodeMap->isCoLocated(otherNodeMap);
}

NABoolean
PushDownCSRequirement::satisfied(const PushDownProperty& other) const
{
  const PushDownCSProperty* x = other.castToPushDownCSProperty();
  if ( x == NULL ) return FALSE;

  // For more details on C1.1/2 and C2.1/2, see the IS "Maintaining Data
  // Integrity for Simple Compound Statements Executing inside DP2".

  // Test colocation (C1.1 or C2.1)
  if ( PushDownColocationRequirement::satisfied(*x) == FALSE )
    return FALSE;

  const PartitioningFunction* partFunc1 = getPartFunc();
  const PartitioningFunction* partFunc2 = x->getPartFunc();

  // Test condition C1.2 -- all tables are not partitioned, or
  // test condition C2.2 -- co-partitioned
  if ( partFunc1->partFuncAndFuncPushDownCompatible(*partFunc2) == FALSE )
     return FALSE;

  // Test condition C2.3 -- access a single partition with
  // the same partition number
  if ( partFunc1->checkSamePartitionNeeded() == TRUE )
  {
     CollIndex n1 = partFunc1->getPartitioningKey().entries();
     CollIndex n2 = partFunc2->getPartitioningKey().entries();

     // Testing the equality on the number of partition key column
     // has been done in partFuncAndFuncPushDownCompatible() so
     // we do not have to worry about it here.

     const SearchKey* skey1 = getSearchKey();
     const SearchKey* skey2 = x->getSearchKey();

     // make sure skey1 and skey2 are identical (on the first n1
     // values).
     if ( skey1 == NULL OR
          skey2 == NULL OR
          skey1->areKeyValuesIdentical(skey2, n1) == FALSE )
        return FALSE;
  }

  return TRUE;
}

const PushDownRequirement* PushDownColocationProperty::makeRequirement() const
{
   return new (CmpCommon::statementHeap())
	PushDownColocationRequirement(map_);
}

const PushDownRequirement* PushDownCSProperty::makeRequirement() const
{
   return new (CmpCommon::statementHeap())
	PushDownCSRequirement(partFunc_, searchKey_);
}


// -----------------------------------------------------------------------
// methods for InputPhysicalProperty
// -----------------------------------------------------------------------

InputPhysicalProperty::~InputPhysicalProperty()
{
}

THREAD_P LookupTable<NABoolean>* ReqdPhysicalProperty::sortOrderTypeCompTab_ = NULL;

// This method initializes a lookup table for comparing
// the synthesized sort order type against any required sort
// order type, to determine if the synthesized sort order type
// satisfies the requirement. See the spec
// "Generating Join Plans with Required Orders", section 6.0,
// for more information.
void ReqdPhysicalProperty::initSortOrderTypeCompTab()
{
  if (sortOrderTypeCompTab_ != NULL)
    return;

  sortOrderTypeCompTab_ =
    new (GetCliGlobals()->exCollHeap())
      LookupTable<NABoolean>(4,6,GetCliGlobals()->exCollHeap());

  sortOrderTypeCompTab_->setValue(0,0,TRUE);
  sortOrderTypeCompTab_->setValue(0,1,FALSE);
  sortOrderTypeCompTab_->setValue(0,2,FALSE);
  sortOrderTypeCompTab_->setValue(0,3,FALSE);
  sortOrderTypeCompTab_->setValue(0,4,FALSE);
  sortOrderTypeCompTab_->setValue(0,5,FALSE);

  sortOrderTypeCompTab_->setValue(1,0,TRUE);
  sortOrderTypeCompTab_->setValue(1,1,TRUE);
  sortOrderTypeCompTab_->setValue(1,2,FALSE);
  sortOrderTypeCompTab_->setValue(1,3,FALSE);
  sortOrderTypeCompTab_->setValue(1,4,TRUE);
  sortOrderTypeCompTab_->setValue(1,5,TRUE);

  sortOrderTypeCompTab_->setValue(2,0,TRUE);
  sortOrderTypeCompTab_->setValue(2,1,FALSE);
  sortOrderTypeCompTab_->setValue(2,2,TRUE);
  sortOrderTypeCompTab_->setValue(2,3,FALSE);
  sortOrderTypeCompTab_->setValue(2,4,TRUE);
  sortOrderTypeCompTab_->setValue(2,5,FALSE);

  sortOrderTypeCompTab_->setValue(3,0,TRUE);
  sortOrderTypeCompTab_->setValue(3,1,FALSE);
  sortOrderTypeCompTab_->setValue(3,2,FALSE);
  sortOrderTypeCompTab_->setValue(3,3,TRUE);
  sortOrderTypeCompTab_->setValue(3,4,FALSE);
  sortOrderTypeCompTab_->setValue(3,5,TRUE);

} // ReqdPhysicalProperty::initSortOrderTypeCompTab()


THREAD_P LookupTable<COMPARE_RESULT>*
  ReqdPhysicalProperty::sortOrderTypeContextCompTab_ = NULL;

// This method initializes a lookup table for comparing
// the required sort order type from the current context against
// the required sort order type of the other context. Used when
// determining if we can steal any of the plans from contexts
// in the current group that have already been optimized.
// See the spec "Generating Join Plans with Required Orders",
// section 8.0, for more information.
void ReqdPhysicalProperty::initSortOrderTypeContextCompTab()
{
  if (sortOrderTypeContextCompTab_ != NULL)
    return;

  sortOrderTypeContextCompTab_ =
    new (GetCliGlobals()->exCollHeap())
      LookupTable<COMPARE_RESULT>(6,6,GetCliGlobals()->exCollHeap());

  sortOrderTypeContextCompTab_->setValue(0,0,SAME);
  sortOrderTypeContextCompTab_->setValue(0,1,MORE);
  sortOrderTypeContextCompTab_->setValue(0,2,MORE);
  sortOrderTypeContextCompTab_->setValue(0,3,MORE);
  sortOrderTypeContextCompTab_->setValue(0,4,MORE);
  sortOrderTypeContextCompTab_->setValue(0,5,MORE);

  sortOrderTypeContextCompTab_->setValue(1,0,LESS);
  sortOrderTypeContextCompTab_->setValue(1,1,SAME);
  sortOrderTypeContextCompTab_->setValue(1,2,INCOMPATIBLE);
  sortOrderTypeContextCompTab_->setValue(1,3,INCOMPATIBLE);
  sortOrderTypeContextCompTab_->setValue(1,4,LESS);
  sortOrderTypeContextCompTab_->setValue(1,5,LESS);

  sortOrderTypeContextCompTab_->setValue(2,0,LESS);
  sortOrderTypeContextCompTab_->setValue(2,1,INCOMPATIBLE);
  sortOrderTypeContextCompTab_->setValue(2,2,SAME);
  sortOrderTypeContextCompTab_->setValue(2,3,INCOMPATIBLE);
  sortOrderTypeContextCompTab_->setValue(2,4,LESS);
  sortOrderTypeContextCompTab_->setValue(2,5,INCOMPATIBLE);

  sortOrderTypeContextCompTab_->setValue(3,0,LESS);
  sortOrderTypeContextCompTab_->setValue(3,1,INCOMPATIBLE);
  sortOrderTypeContextCompTab_->setValue(3,2,INCOMPATIBLE);
  sortOrderTypeContextCompTab_->setValue(3,3,SAME);
  sortOrderTypeContextCompTab_->setValue(3,4,INCOMPATIBLE);
  sortOrderTypeContextCompTab_->setValue(3,5,LESS);

  sortOrderTypeContextCompTab_->setValue(4,0,LESS);
  sortOrderTypeContextCompTab_->setValue(4,1,MORE);
  sortOrderTypeContextCompTab_->setValue(4,2,MORE);
  sortOrderTypeContextCompTab_->setValue(4,3,INCOMPATIBLE);
  sortOrderTypeContextCompTab_->setValue(4,4,SAME);
  sortOrderTypeContextCompTab_->setValue(4,5,UNDEFINED);

  sortOrderTypeContextCompTab_->setValue(5,0,LESS);
  sortOrderTypeContextCompTab_->setValue(5,1,MORE);
  sortOrderTypeContextCompTab_->setValue(5,2,INCOMPATIBLE);
  sortOrderTypeContextCompTab_->setValue(5,3,MORE);
  sortOrderTypeContextCompTab_->setValue(5,4,UNDEFINED);
  sortOrderTypeContextCompTab_->setValue(5,5,SAME);

} // ReqdPhysicalProperty::initSortOrderTypeContextCompTab()

NABoolean ReqdPhysicalProperty::sortOrderTypeReqAndSynthCompatible(
            const SortOrderTypeEnum synthesizedSortOrderType) const
{
  if (sortOrderTypeCompTab_ == NULL)
    initSortOrderTypeCompTab();

  return sortOrderTypeCompTab_->getValue((Int32)synthesizedSortOrderType,
                                         (Int32)sortOrderTypeReq_);
}

COMPARE_RESULT ReqdPhysicalProperty::compareSortOrderTypeReqToReq(
                 const SortOrderTypeEnum otherSortOrderTypeReq) const
{
  if (sortOrderTypeContextCompTab_ == NULL)
    initSortOrderTypeContextCompTab();

  return sortOrderTypeContextCompTab_->getValue((Int32)otherSortOrderTypeReq,
                                                (Int32)sortOrderTypeReq_);
}

NABoolean ReqdPhysicalProperty::dp2SortOrderPartReqAndSynthCompatible(
            const PhysicalProperty *const spp) const
{
  // Should only call this method if there is a dp2SortOrderPartReq
  CMPASSERT(dp2SortOrderPartReq_ != NULL);

  // If there is a dp2SortOrderPartReq, then there had better be
  // a DP2 sort order type requirement, or we had better be in DP2
  DCMPASSERT((sortOrderTypeReq_ == DP2_SOT) OR
             (sortOrderTypeReq_ == DP2_OR_ESP_NO_SORT_SOT) OR
             (location_ == EXECUTE_IN_DP2));

  // If we synthesized a sort order type of DP2, or if we are in DP2,
  // then we must enforce that the required dp2 sort order partitioning
  // function and the synthesized partitioning function are the same.
  if ((spp->getSortOrderType() == DP2_SOT) OR
      (location_ == EXECUTE_IN_DP2))
  {
    CMPASSERT(spp->getDp2SortOrderPartFunc() != NULL);
    if (NOT dp2SortOrderPartReq_->partReqAndFuncCompatible(
              spp->getDp2SortOrderPartFunc()))
      return FALSE;
  }

  return TRUE;

} // ReqdPhysicalProperty::dp2SortOrderPartReqAndSynthCompatible()




COMPARE_RESULT InputPhysicalProperty::compareInputPhysicalProperties(
                 const InputPhysicalProperty& other) const
{
  // Compare Outer Characteristic Outputs
  if (njOuterCharOutputs_ != other.njOuterCharOutputs_)
    return INCOMPATIBLE;

  // Compare Outer Order
  // Only check if both outer order lists are not null
  if ((njOuterOrder_ != NULL) OR (other.njOuterOrder_ != NULL))
  {
    // Is one null, but not the other?
    if ((njOuterOrder_ == NULL) OR (other.njOuterOrder_ == NULL))
      return INCOMPATIBLE;

    // Compare the non-null outer order lists
    // Return INCOMPATIBLE if they are not the same object and
    // are not equivalent.
    if ((njOuterOrder_ != other.njOuterOrder_) AND
        (NOT (*njOuterOrder_ == *(other.njOuterOrder_))))
      return INCOMPATIBLE;
  }

  // Compare Outer Order Partitioning Function
  // Only check if both outer order partitioning functions are not null
  if ((njOuterOrderPartFunc_ != NULL) OR (other.njOuterOrderPartFunc_ != NULL))
  {
    // Is one null, but not the other?
    if ((njOuterOrderPartFunc_ == NULL) OR
        (other.njOuterOrderPartFunc_ == NULL))
      return INCOMPATIBLE;

    // Compare the non-null outer order partitioning functions
    // Return INCOMPATIBLE if they are not the same object and
    // are not equivalent.
    if ((njOuterOrderPartFunc_ != other.njOuterOrderPartFunc_) AND
        (NOT njOuterOrderPartFunc_->
              comparePartFuncToFunc(*(other.njOuterOrderPartFunc_))))
      return INCOMPATIBLE;
  }

  // Compare DP2 Outer Order Partitioning Function
  // Only check if both DP2 outer order partitioning functions are not null
  if ((njDp2OuterOrderPartFunc_ != NULL) OR
      (other.njDp2OuterOrderPartFunc_ != NULL))
  {
    // Is one null, but not the other?
    if ((njDp2OuterOrderPartFunc_ == NULL) OR
        (other.njDp2OuterOrderPartFunc_ == NULL))
      return INCOMPATIBLE;

    // Compare the non-null DP2 outer order partitioning functions
    // Return INCOMPATIBLE if they are not the same object and
    // are not equivalent.
    if ((njDp2OuterOrderPartFunc_ != other.njDp2OuterOrderPartFunc_) AND
        (NOT njDp2OuterOrderPartFunc_->
              comparePartFuncToFunc(*(other.njDp2OuterOrderPartFunc_))))
      return INCOMPATIBLE;
  }

  if (assumeSortedForCosting_ != other.assumeSortedForCosting_)
    return INCOMPATIBLE;
  if (explodedOcbJoinForCosting_ != other.explodedOcbJoinForCosting_)
    return INCOMPATIBLE;
  // If we get here the input physical properties must both be the same
  return SAME;

} // InputPhysicalProperty::compareInputPhysicalProperties()


COMPARE_RESULT 
PushDownColocationRequirement::compare(const PushDownRequirement &other) const
{
  const PushDownColocationRequirement * otherP = 
    other.castToPushDownColocationRequirement();
  if (otherP == NULL)
    return INCOMPATIBLE;

  // Passed argument could be PushDownCSRequirement object,
  // make sure it is not (i.e otherCS must be NULL).
  const PushDownCSRequirement * otherCS =
    other.castToPushDownCSRequirement();
  if (otherCS != NULL)
    return INCOMPATIBLE;

  const PushDownColocationRequirement & otherReq = *otherP;

  if (map_ == otherReq.map_)
    return SAME;
  else if ( map_ == NULL )
    return INCOMPATIBLE;
  else if (otherReq.map_ == NULL)
    return INCOMPATIBLE;
  else if( map_->isCoLocated(otherReq.map_) )
    return SAME;
  else
    return INCOMPATIBLE;
}

COMPARE_RESULT 
PushDownCSRequirement::compare(const PushDownRequirement &other) const
{
  COMPARE_RESULT result = 
    PushDownColocationRequirement::compare(other);

  const PushDownCSRequirement * otherP =
    other.castToPushDownCSRequirement();
  if (otherP == NULL)
    return INCOMPATIBLE; 
  const PushDownCSRequirement & otherReq = *otherP;

  if(partFunc_ == otherReq.partFunc_)
  {
  } 
  else if(partFunc_ == NULL)
    return INCOMPATIBLE;
  else if(otherReq.partFunc_ == NULL)
    return INCOMPATIBLE; 
  else 
    result = combine_compare_results(result, 
	     partFunc_->comparePartFuncToFunc(*otherReq.partFunc_));


  if(searchKey_ == otherReq.searchKey_)
  {
  } 
  else if(searchKey_ == NULL)
    return INCOMPATIBLE; 
  else if(otherReq.searchKey_ == NULL)
    return INCOMPATIBLE; 
  else 
  {
    CollIndex numValues = searchKey_->getBeginKeyValues().entries();
    CollIndex otherNumValues = otherReq.searchKey_->getBeginKeyValues().entries();

    if(numValues != otherNumValues) {
      return INCOMPATIBLE;
    }

    if(NOT searchKey_->areKeyValuesIdentical(otherReq.searchKey_, numValues))
      return INCOMPATIBLE;
  }

  return result;
}

