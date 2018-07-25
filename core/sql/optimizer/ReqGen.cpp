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
 *****************************************************************************
 *
 * File:         ReqGen.cpp
 * Description:  Class to generate required physical properties for a
 *               child of a RelExpr node
 *
 * Created:      2/5/98
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"
#include "ReqGen.h"
#include "GroupAttr.h"
#include "Globals.h"

RequirementGenerator::RequirementGenerator(
     const ExprGroupId &groupForRequirement,
     const ReqdPhysicalProperty *startRequirements)
 : startRequirements_                  (startRequirements),
   ga_                                  (groupForRequirement.getGroupAttr()),
   stillFeasible_                       (TRUE),
   startRequirementsHaveBeenProcessed_  (startRequirements == NULL),
   generatedRequirement_                (NULL),
   generatedPartitioningRequirement_    (NULL),
   generatedPartitioningReqIsValid_     (FALSE),
   addedSortKey_                        (NULL),
   sortKeyIsUnique_                     (FALSE),
   removeStartSortKey_                  (FALSE),
   addedArrangement_                    (NULL),
   removeStartArrangement_              (FALSE),
   partKeyHasBeenAdded_                 (FALSE),
   addedNumberOfPartitions_             (ANY_NUMBER_OF_PARTITIONS),
   addedNumOfPartsAllowedDeviation_     (
     CURRSTMT_OPTDEFAULTS->numberOfPartitionsDeviation()),
   addedPartitioningRequirement_        (NULL),
   removeStartPartitioningRequirements_ (FALSE),
   locationHasBeenAdded_                (FALSE),
   removeStartLocation_                 (FALSE),
   addedSortOrderTypeReq_               (NO_SOT),
   addedDp2SortOrderPartReq_            (NULL),
   addedSkewProperty_                   (ANY_SKEW_PROPERTY),
   skewPropertyAdded_                   (FALSE),
   requireNoESPExchange_                (FALSE),
   requireHash2Only_                    (FALSE)
{
  // For the more simple requirements, just initialize them with
  // the start requirements or the default values. That means these
  // values don't get processed in processStartRequirements().
  if (startRequirements)
    {
      availableCPUs_    = startRequirements->getCountOfAvailableCPUs();
      pipelinesPerCPU_	= startRequirements->getCountOfPipelines() /
                                                            availableCPUs_;
      costWeight_ 	= startRequirements->getCostWeight();
      perfGoal_ 	= startRequirements->getPerformanceGoal();
      logicalPartReq_   = startRequirements->getLogicalPartRequirement();

      logicalOrderOrArrangement_ =
        startRequirements->getLogicalOrderOrArrangementFlag();

      pushDownRequirement_ = startRequirements->getPushDownRequirement();
      requireNoESPExchange_ = startRequirements->getNoEspExchangeRequirement();
	  requireOcbEnabledCosting_ = startRequirements->getOcbEnabledCostingRequirement();
    }
  else
    {
      availableCPUs_    = DEFAULT_SINGLETON;
      pipelinesPerCPU_	= DEFAULT_SINGLETON;
      costWeight_ 	= CURRSTMT_OPTDEFAULTS->getDefaultCostWeight();
      perfGoal_ 	= CURRSTMT_OPTDEFAULTS->getDefaultPerformanceGoal();
      logicalPartReq_   = NULL;
      logicalOrderOrArrangement_ = FALSE;
      pushDownRequirement_ = 0;
	  requireOcbEnabledCosting_ = 0;
    }
}

void RequirementGenerator::addSortKey(const ValueIdList &additionalSortKey,
                        SortOrderTypeEnum sortOrderTypeReq,
                        PartitioningRequirement* dp2SortOrderPartReq)
{
  // This applies to adding both orders and arrangements:
  //
  // a) A new order requirement conflicts with an existing order
  //    requirement if one of the two requirements (simplified) values
  //    are not a prefix of the other (including the case where the
  //    two reqs are identical).
  // b) A new arrangement requirement conflicts with an existing
  //    order requirement if its simplified values are not a leading
  //    prefix of the order requirement, and if the order required
  //    columns are not covered by the required arrangement columns.
  // c) A new order requirement conflicts with an existing
  //    non-empty arrangement requirement if its (simplified) values are
  //    not a subset of the existing (simplified) arrangement requirement,
  //    and if the order required columns are not covered by
  //    the required arrangement columns.
  // d) A new arrangement requirement conflicts with an existing
  //    non-empty arrangement requirement if its (simplified) values are
  //    different from the existing arrangement requirement, but see e)
  // e) A new arrangement requirement that is a proper superset or subset
  //    of an existing arrangement requirement may be added if the smaller
  //    of the new and existing requirements is converted into a new order
  //    requirement. Converting the smaller arrangement requirement into
  //    an order requirement is a way to pass conditions b) and d).
  //
  // Two things can be done to simplify a required order or arrangement:
  // BUT, we can't do them now until we make changes in the "satisfied"
  // method and in merge join. So (f) and (g) are not done now.
  //
  // f) If the group attributes have a cardinality constraint of at
  //    most one result row, then we can ignore any required order or
  //    arrangement, no matter how weird, because a single row is
  //    ordered by basically anything.
  // g) If a prefix of the required sort key or a subset of an arrangement
  //    is unique, then it is ok to ignore all other values of the order
  //    requirement and the entire arrangement requirement
  //    because the result will automatically be ordered by any
  //    combination of such columns (there will always be only
  //    one row for each unique key column combination, and that
  //    one row is sorted by anything, just like in the first
  //    item). In fact, the GroupAttributes::isUnique() method
  //    is written such that it can handle both f) and g) with only a
  //    single check.
  //
  // Checks a) and c) are done here, checks b), d) and e) are done in
  // RequirementGenerator::addArrangement(), checks f) and g) are done
  // in both places (BUT NOT RIGHT NOW).
  //
  // The above rules and the simplification of order expressions done
  // in ItemExpr::simplifyOrderExpr() allow some nice optimizations.
  // Example: "ORDER BY a, b-3, a desc, 4-b, c, d, e" is equivalent to
  // "ORDER BY a, b, c" if c is a unique column, and the code below
  // will find this. The generated requirement, however, may also be
  // "ORDER BY a, b-3, c".


  // Do not bother if additionalSortKey is an empty list
  if (additionalSortKey.entries() == 0)
    return;

  // Add the sort order type requirement and associated Dp2 sort order
  // partitioning requirement
  addSortOrderTypeReq(sortOrderTypeReq, dp2SortOrderPartReq);

  CollIndex existingEntries = 0;
  CollIndex i;

  // Before doing any processing, must eliminate any duplicates from
  // the additional sort key being added.
  ValueIdList noDupsAdditionalSortKey;
  ValueIdList simpAdditionalSortKey;
  for (i = 0; i < additionalSortKey.entries(); i++)
  {
    ValueId simpVid = additionalSortKey[i].getItemExpr()->
                     simplifyOrderExpr()->getValueId();

    // Insert the current new value only if it is not
    // dependent on a previous new value  (this is checked
    // by looking up the simplified new value in the existing set of
    // simplified new values).
    if (NOT simpAdditionalSortKey.contains(simpVid))
    {
      noDupsAdditionalSortKey.insert(additionalSortKey[i]);
      simpAdditionalSortKey.insert(simpVid);
    }
  }


  if (addedSortKey_ == NULL)
    {
      // allocate a new ValueIdList for the required order
      // (it may turn out that the allocated ValueIdList stays empty)
      addedSortKey_ = new(CmpCommon::statementHeap()) ValueIdList;
    }
  else
    {
      // check a) (see comment above)
      // make sure the already added sort key columns match the new
      // ones

      existingEntries = addedSortKey_->entries();
      for (i = 0; i < existingEntries AND stillFeasible_; i++)
	if (i < noDupsAdditionalSortKey.entries())
	  {
	    if ((*addedSortKey_)[i] != noDupsAdditionalSortKey[i])
	      {
		ItemExpr *ie1      = (*addedSortKey_)[i].getItemExpr();
		ItemExpr *ie2      = noDupsAdditionalSortKey[i].getItemExpr();
                // sameOrder method compares the SIMPLIFIED values
		OrderComparison co = ie1->sameOrder(ie2);

		if (co != SAME_ORDER)
		  {
		    // the required order conflicts with an already
		    // existing order requirement
		    stillFeasible_ = FALSE;
		  }
	      }
	  }
    } // end check (a)

  // Insert new sort key columns that go beyond the already added columns,
  // if there are any and if the order does not already extend over a
  // unique column combination.
  for (i = existingEntries;
       i < noDupsAdditionalSortKey.entries() AND NOT sortKeyIsUnique_;
       i++)
    {
      ValueId noDupsVid = noDupsAdditionalSortKey[i];
      ValueId simpVid = simpAdditionalSortKey[i];

      // perform checks f) and g), note that simpleSortCols_ may be empty!
      // COMMENTED OUT UNTIL "satisfied" METHOD CHANGES ARE MADE,
      // and changes to merge join.
      //if (ga_->isUnique(ValueIdSet(simpleSortCols_)))
      //{
          // The order key cols so far are already unique, no need to
          // add another column. In fact, we can now accept even
          // seemingly incompatible ordering requirements for additional
          // columns!
        //sortKeyIsUnique_ = TRUE;
      //}
      //else
      //{
          addedSortKey_->insert(noDupsVid);

          // remember the simplified order value for checks b) and c)
          simpleSortCols_.insert(simpVid);
      //}
    }

  // check c) (see comment above)
  // check for a conflict with an existing arrangement requirement
  if (addedArrangement_ != NULL AND addedArrangement_->entries() > 0)
  {
    Int32 count = simpleSortCols_.prefixCoveredInSet(simpleArrangedCols_);
    // The entire simplified required arrangement must be a leading
    // prefix of the simplified required sort order, or all the
    // required sort order columns must be covered by the
    // arrangement columns. i.e. if the required sort order is ABC,
    // the arrangement must be something like AB or ABCD.
    if (NOT (((CollIndex)count == simpleArrangedCols_.entries()) OR
             ((CollIndex)count == simpleSortCols_.entries())))
      stillFeasible_ = FALSE;
  }

} // addSortKey

void RequirementGenerator::makeSortKeyFeasible(
          ValueIdList &proposedSortKey)
{
  CollIndex i = 0;
  NABoolean match = TRUE;
  ValueId svid;

  processStartRequirements();

  // Check for compatibility with an existing sort requirement
  // The n columns of the existing sort key requirement and the first
  // n columns of the proposed sort key must match exactly.
  if (addedSortKey_ != NULL)
  {
    while ((i < addedSortKey_->entries()) AND
           (i < proposedSortKey.entries()) AND match)
    {
      if ((*addedSortKey_)[i] != proposedSortKey[i])
      {
        ItemExpr *ie1      = (*addedSortKey_)[i].getItemExpr();
        ItemExpr *ie2      = proposedSortKey[i].getItemExpr();
        // sameOrder method compares the SIMPLIFIED values
        OrderComparison co = ie1->sameOrder(ie2);

        if (co != SAME_ORDER)
          match = FALSE;
      }
      if (match)
        i++;
    } // end while
    // If we found a proposed sort key column that did not match its
    // counterpart in the existing sort key, we must remove it and
    // all subsequent proposed sort key columns.
    if (NOT match)
    {
      Int32 lastEntry = (Int32)proposedSortKey.entries() - 1;
      while (lastEntry >= (Int32)i)
      {
        proposedSortKey.removeAt(lastEntry--);
      }
    }
  } // end if there was a required sort order
  else if (addedArrangement_ != NULL)
  {
    // The first n columns of the simplified proposed sort key must be
    // part of the n columns of the simplified existing arrangement
    // requirement.
    while ((i < simpleArrangedCols_.entries()) AND
           (i < proposedSortKey.entries()) AND match)
    {
      svid = proposedSortKey[i].getItemExpr()->
               simplifyOrderExpr()->getValueId();
      if (NOT simpleArrangedCols_.contains(svid))
        match = FALSE;
      if (match)
        i++;
    }
    // If we found a proposed sort key column that is not in the
    // required arrangement, we must remove it and all subsequent
    // proposed sort key columns.
    if (NOT match)
    {
      Int32 lastEntry = (Int32)proposedSortKey.entries() - 1;
      while (lastEntry >= (Int32)i)
      {
        proposedSortKey.removeAt(lastEntry--);
      }
    }
  }  // end if there is already a required arrangement

} // end makeSortKeyFeasible()

void RequirementGenerator::makeArrangementFeasible(
          ValueIdSet &proposedArrangement)
{
  ValueId svid;

  processStartRequirements();

  // The arrangement is certainly feasible if there are no existing
  // sort key or arrangement requirements.
  if ((addedSortKey_ == NULL) AND (addedArrangement_ == NULL))
    return;

  // a simplified version of proposedArrangement
  ValueIdSet simpleProposedArrangement;

  for (ValueId vid = proposedArrangement.init();
       proposedArrangement.next(vid);
       proposedArrangement.advance(vid))
  {
    simpleProposedArrangement +=
      vid.getItemExpr()->simplifyOrderExpr()->getValueId();
  }

  // If we have already added the proposed arrangement, then it
  // must be feasible, so just return.
  if (simpleArrangedCols_ == simpleProposedArrangement)
    return;

  // is there a required sort order?
  if (addedSortKey_ != NULL)
  {
    Int32 count = simpleSortCols_.prefixCoveredInSet(simpleProposedArrangement);
    // The entire simplified proposed arrangement must be a leading
    // prefix of the simplified required sort order, or all the
    // simplified required sort order columns must be covered by the
    // simplified proposed arrangement columns. i.e. if the required
    // sort order is ABC, the proposed arrangement must be something like
    // AB or ABCD.
    if (NOT (((CollIndex)count == simpleProposedArrangement.entries()) OR
             ((CollIndex)count == simpleSortCols_.entries())))
    {
      // Some or all of the proposed arrangement columns are not compatible
      // with the required sort order columns. i.e. if the required sort
      // order is ABC, and the proposed arrangement is B (no compatible
      // columns) or AC (C is not compatible).
      if (count == 0)
        // All of them are not compatible. Get rid of all of them.
        // This will result in the empty set. This is ok. It means
        // that when we ask for that arrangement it will not cover
        // a prefix of the required sort order and so we will give up.
        proposedArrangement.clear();
      else
      {
        // Some are not compatible. Figure out which ones and get rid of them.
        ValueIdList coveredSimpleSortCols(simpleSortCols_);
        coveredSimpleSortCols.removeUncoveredSuffix(simpleProposedArrangement);

        for (ValueId v = proposedArrangement.init();
                         proposedArrangement.next(v);
                         proposedArrangement.advance(v))
        {
          svid = v.getItemExpr()->simplifyOrderExpr()->getValueId();
          if (NOT coveredSimpleSortCols.contains(svid))
            proposedArrangement -= v;
        }
      } // end else count != 0
    } // end if the proposed arrangement was not ok
  } // end if there was a required sort order
  else if (addedArrangement_ != NULL)
  {
    // Already have a required arrangement. If one of the arrangements
    // is not a subset of the other, then we must remove the columns
    // from the proposed arrangement that are not in the already added
    // arrangement.
    if (NOT simpleArrangedCols_.contains(simpleProposedArrangement) AND
        NOT simpleProposedArrangement.contains(simpleArrangedCols_))
    {
      // Remove columns from the proposed arrangement. This may
      // result in the empty set. This is ok - it means when we ask
      // for that arrangement that it will not be compatible with
      // the existing arrangement and we will give up.

      simpleProposedArrangement.intersectSet(simpleArrangedCols_);
      if (simpleProposedArrangement.isEmpty())
        // Existing required arrangement and proposed arrangement
        // are disjoint. Remove all columns from the proposed
        // arrangement.
        proposedArrangement.clear();
      else // Some columns in common.
        for (ValueId v = proposedArrangement.init();
                         proposedArrangement.next(v);
                         proposedArrangement.advance(v))
        {
          svid = v.getItemExpr()->simplifyOrderExpr()->getValueId();
          if (NOT simpleProposedArrangement.contains(svid))
            proposedArrangement -= v;
        }
    } // end if arrangements are not subsets
  }  // end if there is already a required arrangement

} // end makeArrangementFeasible()


void RequirementGenerator::addArrangement(
       const ValueIdSet &additionalArrangementParam,
       SortOrderTypeEnum sortOrderTypeReq,
       PartitioningRequirement* dp2SortOrderPartReq)
{
  // Do not bother if additionalArrangementParam is an empty set
  if (additionalArrangementParam.entries() == 0)
    return;

  // Add the sort order type requirement and associated Dp2 sort order
  // partitioning requirement
  addSortOrderTypeReq(sortOrderTypeReq, dp2SortOrderPartReq);

  // local copy of the new arrangement
  ValueIdSet additionalArrangement;
  // a simplified version of additionalArrangement
  ValueIdSet simpleAddtnlArrangement;
  ValueId svid;

  for (ValueId vid = additionalArrangementParam.init();
       additionalArrangementParam.next(vid);
       additionalArrangementParam.advance(vid))
  {
    svid = vid.getItemExpr()->simplifyOrderExpr()->getValueId();
    // Only keep the current value id of the new arrangement
    // if its simplified form is not already in the simplified
    // form of the arrangement.
    if (NOT simpleAddtnlArrangement.contains(svid))
    {
      additionalArrangement += vid;
      simpleAddtnlArrangement += svid;
    }
  }

  // Perform checks f) and g), eliminate redundant requests for
  // arrangements.
  // COMMENTED OUT UNTIL "satisfied" METHOD CHANGES ARE MADE,
  // and changes to merge join.
  //if (ga_->isUnique(simpleAddtnlArrangement))
  //{
  //  additionalArrangement.clear();
  //  simpleAddtnlArrangement.clear();
  //}

  // perform check b) (see comment in addSortKey())
  if ((addedSortKey_ != NULL) AND
      // Don't need to perform check if there already is an arrangement
      // that is the same as the one we want to add, since it must have
      // already passed the checks.
      ((addedArrangement_ == NULL) OR
       (simpleArrangedCols_ != simpleAddtnlArrangement)))
  {
    Int32 count = simpleSortCols_.prefixCoveredInSet(simpleAddtnlArrangement);
    // The entire arrangement must be a leading prefix of the
    // required sort order, or all the required sort order columns must
    // be covered by the arrangement columns. i.e. if the required
    // sort order is ABC, the arrangement must be something like
    // AB or ABCD.
    if (NOT (((CollIndex)count == simpleAddtnlArrangement.entries()) OR
             ((CollIndex)count == simpleSortCols_.entries())))
      stillFeasible_ = FALSE;
  }

  // perform check d) (see comment in addSortKey())
  if (stillFeasible_ AND
      (addedArrangement_  != NULL) AND
      (simpleArrangedCols_ != simpleAddtnlArrangement))
    {
      // failed check d), now try the conversion described in check e)
      // and see whether that got around the problem

      ValueIdSet arrToConvert;
      ValueIdSet simpleArrToConvert;

      if (simpleArrangedCols_.contains(simpleAddtnlArrangement))
	{
	  // convert additionalArrangement into a required order and don't
	  // add any new arrangement
	  arrToConvert = additionalArrangement;
	  additionalArrangement.clear();
	  simpleArrToConvert = simpleAddtnlArrangement;
	  simpleAddtnlArrangement.clear();
	}
      else if (simpleAddtnlArrangement.contains(simpleArrangedCols_))
	{
	  // remove addedArrangement and convert it into a required
	  // order, then add additionalArrangement as a new arrangement
	  arrToConvert = *addedArrangement_;
	  addedArrangement_->clear();
          simpleArrToConvert = simpleArrangedCols_;
          simpleArrangedCols_.clear();
	}
      else
	stillFeasible_ = FALSE; // conversion from check e) was not possible

      if (stillFeasible_)
	{
	  // Now try the conversion of step e). There are many ways to
	  // convert an arrangement into a sort order, pick a way that
	  // does not conflict with an existing order requirement

	  // since arrToConvert contains all the columns of the
	  // required order (if any), just start with the required
	  // order and add all values of arrToConvert that are not in
	  // the already required order
	  ValueIdList orderFromArr;

          ValueIdSet sortColsAsSet(simpleSortCols_);

	  // we should already have done check b)
	  CMPASSERT(simpleArrToConvert.contains(sortColsAsSet) OR
		    sortColsAsSet.contains(simpleArrToConvert));

	  if (addedSortKey_ != NULL)
	  {
	    orderFromArr = *addedSortKey_;

	    // Remove all the columns that addedSortKey_ is based upon
	    // from arrToConvert, so we can add the rest below.
            // NOTE: This is no longer necessary. Method addSortKey() now eliminates
            // all duplicates from the sort key being added before proceeeding.
            // First: Remove them from simpleArrToConvert.
	    simpleArrToConvert -= sortColsAsSet;
            // Second: Now, remove all items from arrToConvert whose
            // simplified form is no longer in simpleArrToConvert.
            for (ValueId v = arrToConvert.init();
                             arrToConvert.next(v);
                             arrToConvert.advance(v))
            {
              svid = v.getItemExpr()->simplifyOrderExpr()->getValueId();
              if (NOT simpleArrToConvert.contains(svid))
                arrToConvert -= v;
            }
	  }

	  // Now take care of those arrangement columns that are not
	  // yet a part of the required order but that need to be
	  // converted into an order. Simply take them in the sequence
	  // of their value ids and make them all ascending.
	  for (ValueId cvid = arrToConvert.init();
	       arrToConvert.next(cvid);
	       arrToConvert.advance(cvid))
	    {
	      orderFromArr.insert(cvid);
	    }

	  // now the actual conversion
	  addSortKey(orderFromArr,
                     sortOrderTypeReq,
                     dp2SortOrderPartReq);

	  // there isn't any reason why this should not have worked,
	  // we did all the necessary checks before
	  CMPASSERT(stillFeasible_);
	} // try conversion of check e)
    } // check d)

  // Need to add the new arrangement if it is still feasible, has
  // not been converted to an order, and is not equal to the
  // existing arrangement. Note that for all this to be true there
  // must not have been an existing arrangement, or the new
  // arrangement must be a superset of the existing arrangement.
  if (stillFeasible_ AND
      NOT additionalArrangement.isEmpty() AND
      ((addedArrangement_ == NULL) OR
       (simpleArrangedCols_ != simpleAddtnlArrangement)))
    {
      if (addedArrangement_ == NULL)
	{
	  addedArrangement_ =
	    new (CmpCommon::statementHeap())
              ValueIdSet(additionalArrangement);
          simpleArrangedCols_ = simpleAddtnlArrangement;
	}
      else
	{
	  // We can only come here if the new arrangement is a
	  // superset of an already added one.  This means that we have
	  // converted the old "addedArrangement_" to an order
          // and then cleared the old arrangement.
          // So, we just need to add the new arrangement.
	  CMPASSERT(addedArrangement_->isEmpty());
	  *addedArrangement_ = additionalArrangement;
          simpleArrangedCols_ = simpleAddtnlArrangement;
	}
    }

} // addArrangement

// combine partKey1 with partKey2 heuristically
ValueIdSet RequirementGenerator::combinePartKeysHeuristically(
     const ValueIdSet& partKey1,
     const ValueIdSet& partKey2)
  // if partKey1 == partKey2 then return either one, else
  // when PARTITIONING_SCHEME_SHARING is 
  //   0 (the default) means complete sharing (current behavior)
  //   1 means require one set to contain the other
  //   otherwise disable sharing
{
  ValueIdSet result(partKey1);

  // initialize the result with the intersection of the keys
  result.intersectSet(partKey2);

  Lng32 pssc = CURRSTMT_OPTDEFAULTS->partitioningSchemeSharing();
  if (pssc == 0)
    return result;

  // check whether either key is a subset of the other
  NABoolean key1IsASubsetOfKey2 = (partKey1.entries() == result.entries());
  NABoolean key2IsASubsetOfKey1 = (partKey2.entries() == result.entries());

  // if both partitioning keys are the same (are subsets of
  // each other), then there is no need to apply heuristics
  if (key1IsASubsetOfKey2 AND key2IsASubsetOfKey1)
    return result;

  // With PSSC set to 1 we require that one key being a superset of the other
  if ((pssc == 1) AND
	  (key1IsASubsetOfKey2 OR key2IsASubsetOfKey1))
    return result; 

  // add cases where we shouldn't use the result
  // make the result empty means only a serial plan will be acceptable
  result.clear();
  // don't set stillFeasible_ = FALSE; because that would cause errors with
  // operators that need to execute in the master and that have 2 other 
  // operators on top of them

  return result;
}

void RequirementGenerator::addPartitioningKey(
     const ValueIdSet &partKey)
{
  ValueIdSet simplifiedPartKey(partKey);

  if (CmpCommon::getDefault(COMP_BOOL_111) != DF_ON)
  {
    // consider query
    //
    //  SELECT [last 1]
    //    (POS.WM_FISCAL_YR - 1900)||'01',
    //    POS.STORE_NBR,
    //    SUM( POS.YRLY_SALES )
    //  FROM
    //    WM_USER.SKU_YRLY_POS POS,
    //    WM_HOLD.WM_ITEM073 ITEMP < < +cardinality 2.8e6>>,
    //    WM_HOLD.WM_CLUB STEMP < < +cardinality 0.3e1>>
    //  WHERE
    //    STEMP.UID = '6D9E40D2WMH6001' AND
    //    ITEMP.UID = 'E7377E75WMH6001' AND
    //    POS.STORE_NBR = STEMP.CLUB AND
    //    POS.ITEM_NBR = ITEMP.MDS_FAM_ID AND
    //    POS.WM_FISCAL_YR = 2006
    //  GROUP BY 1,2
    //  ORDER BY POS.STORE_NBR ;
    //
    //  expression (POS.WM_FISCAL_YR - 1900)||'01' is a constant for
    //  our purpose, because POS.WM_FISCAL_YR has an equality predicate
    //  against a constant i.e. predicate POS.WM_FISCAL_YR = 2006

    NABoolean considerConstExpressions = TRUE;
    // remove constant values from the partitioning key
    simplifiedPartKey.removeConstExprReferences(considerConstExpressions);
  }

  // check whether a partitioning key has been added before
  if (partKeyHasBeenAdded_)
    {
      // if there is already a partitioning key, build the intersection
      // NOTE: this may result in the empty set being required as a
      // partitioning key (that's ok and is equivalent to a requirement
      // for exactly one partition)
      //
      // We use heuristics to decide whether the intersection really
      // forms a useful partitioning requirement. For example, combining
      // (L_ORDERKEY, N_NATIONKEY) and (N_NATIONKEY, C_CUSTKEY) results
      // in a partitioning requirement for N_NATIONKEY - probably not a
      // good idea, given that there are only 25 nations in TPC-H.
      addedPartKey_ = combinePartKeysHeuristically(addedPartKey_,
                                                   simplifiedPartKey);
    }
  else
    {
      // just copy the part key if this is the first time we add one
      // (note that no requirement is not the same as the requirement
      // for an empty part key!!!!!)
      partKeyHasBeenAdded_ = TRUE;
      addedPartKey_ = simplifiedPartKey;
    }
  generatedPartitioningReqIsValid_ = FALSE;
  // resolve possible conflicts with fully-specified partitioning functions
  // later
}

void RequirementGenerator::addNumOfPartitions(
     Lng32 newNumberOfPartitions,
     float newNumOfPartsAllowedDeviation)
{
  if (newNumberOfPartitions == ANY_NUMBER_OF_PARTITIONS)
    return;

  CMPASSERT(newNumberOfPartitions > 0);

  if (addedNumberOfPartitions_ == ANY_NUMBER_OF_PARTITIONS)
    {
      addedNumberOfPartitions_ = newNumberOfPartitions;
      addedNumOfPartsAllowedDeviation_ = newNumOfPartsAllowedDeviation;
      generatedPartitioningReqIsValid_ = FALSE;
    }
  else
    {
      if (addedNumberOfPartitions_ != newNumberOfPartitions)
        {
          // conflicting requirements for number of partitions
          // $$$$ could check whether the numbers are approximately the same
          // and remember a range
          stillFeasible_ = FALSE;
        }

      // go with the smaller of the allowed deviations
      if (newNumOfPartsAllowedDeviation < addedNumOfPartsAllowedDeviation_)
      {
        addedNumOfPartsAllowedDeviation_ = newNumOfPartsAllowedDeviation;
        generatedPartitioningReqIsValid_ = FALSE;
      }
    }

  // resolve possible conflicts with fully-specified partitioning functions
  // and requirements for a partitioning key later
}

void RequirementGenerator::addPartRequirement(
     PartitioningRequirement *pr)
{

  if (pr->isRequirementFuzzy())
  {
    if (NOT pr->getPartitioningKey().isEmpty())
      addPartitioningKey(pr->getPartitioningKey());
    if (pr->getCountOfPartitions() != ANY_NUMBER_OF_PARTITIONS)
    {
      if (pr->isRequirementApproximatelyN())
      {
        addNumOfPartitions(pr->getCountOfPartitions(),
                           pr->castToRequireApproximatelyNPartitions()->
                               getAllowedDeviation());
        requireHash2Only_ = pr->castToRequireApproximatelyNPartitions()->
                                isRequireHash2Only();
      }
      else
        addNumOfPartitions(pr->getCountOfPartitions());
    }
  }
  else if (addedPartitioningRequirement_ != NULL)
    {
      if (addedPartitioningRequirement_->comparePartReqToReq(pr) != SAME)
        stillFeasible_ = FALSE;
    }
  else
    {
      addedPartitioningRequirement_ = pr;

      generatedPartitioningReqIsValid_ = FALSE;
      // don't worry about possible conflicts with added partitioning
      // keys and numbers of partitions quite yet
    }
} // addPartRequirement

void
RequirementGenerator::makeSkewReqFeasible(PartitioningRequirement* partR)
{
  if ( skewPropertyAdded_ == FALSE || partR == NULL )
      return;

  // check part requirement partR. If it is fuzzy, nothing we can check
  // other than set the skew property
  // to it.
  if (partR -> isRequirementFuzzy() ) {
     const skewProperty& skInFuzzyR =
          ((FuzzyPartitioningRequirement*)partR)->getSkewProperty();

     if ( skInFuzzyR.isAnySkew() ) {
        ((FuzzyPartitioningRequirement*)partR) ->
              setSkewProperty(addedSkewProperty_);
     } else {
        if ( NOT (skInFuzzyR == addedSkewProperty_) )
           stillFeasible_ = FALSE;
     }

     return;
  } else {
    // check the part func contained in the (fully specified) partR
    const FullySpecifiedPartitioningRequirement* fs_req =
         partR->castToFullySpecifiedPartitioningRequirement();

    if ( fs_req ) {

         PartitioningFunction* partFunc = fs_req->getPartitioningFunction();
         if ( partFunc != NULL ) {

           const SkewedDataPartitioningFunction* skpf =
              partFunc->castToSkewedDataPartitioningFunction();

           // The contained partfunc should be a SkewedData partfunc.
           // If not, declare it not feasible right here
           if ( skpf == NULL )
               stillFeasible_ = FALSE;
           else {
               // Make sure its skew property is the same as the
               // one set to the ReqGen.
               if (NOT (skpf->getSkewProperty() == addedSkewProperty_ ))
                 stillFeasible_ = FALSE;
            }
         }
     }
  }
}

void
RequirementGenerator::addSkewRequirement(skewProperty::skewDataHandlingEnum x,
                                         SkewedValueList* skList,
					 NABoolean broadcastOneRow)
{
  if ( skewPropertyAdded_ == TRUE ) {
     skewProperty tempSk;
     tempSk.setIndicator(x);
     tempSk.setBroadcastOneRow(broadcastOneRow);
     tempSk.setSkewValues(skList);

     if (NOT (tempSk == addedSkewProperty_)) {
       stillFeasible_ = FALSE;
       return;
     }
  }

  addedSkewProperty_.setIndicator(x);
  addedSkewProperty_.setBroadcastOneRow(broadcastOneRow);
  addedSkewProperty_.setSkewValues(skList);
  skewPropertyAdded_ = TRUE;

  if ( generatedPartitioningReqIsValid_ == TRUE AND
       generatedPartitioningRequirement_
     )
    makeSkewReqFeasible(generatedPartitioningRequirement_);
}

void RequirementGenerator::addLocationRequirement(
     PlanExecutionEnum loc)
{
  if (NOT locationHasBeenAdded_)
    {
      addedLocation_        = loc;
      locationHasBeenAdded_ = TRUE;
    }
  else
  {
    if (loc != addedLocation_)
    {
      if ( (addedLocation_ == EXECUTE_IN_MASTER_OR_ESP) AND
           ( (loc == EXECUTE_IN_MASTER) OR (loc == EXECUTE_IN_ESP) )
         )
      {
        // change location to more specific one
        addedLocation_ = loc;
      }
      else
      { // The only two cases we want to keep stillFeasible_ value
        // as TRUE are: loc is EXECUTE_IN_MASTER_OR_ESP and
        // (addedLocation_ is EXECUTE_IN_MASTER or EXECUTE_IN_ESP)
        // otherwisw we change it to FALSE
        if ( (loc != EXECUTE_IN_MASTER_OR_ESP) OR
             (addedLocation_ == EXECUTE_IN_DP2) )
        {
          // incompatible location requirements
          stillFeasible_ = FALSE;
        }
      }

    } // if(loc != addedLocation_)

  } // else(addedLocation was TRUE)

}

void RequirementGenerator::addLogicalPartRequirement(
  LogicalPartitioningRequirement *pr)
{
  // it should never happen that we add two different logical partitioning
  // requirements, only one DP2 exchange should do that once
  CMPASSERT(logicalPartReq_ == NULL);
  logicalPartReq_ = pr;
}

void RequirementGenerator::removeSortKey()
{
  addedSortKey_ = NULL;
  sortKeyIsUnique_ = FALSE;
  removeStartSortKey_ = TRUE;
  simpleSortCols_.clear();

  // If there is no added arrangement requirement, we can also remove any
  // sort order type requirements.
  if (addedArrangement_ == NULL)
  {
    addedSortOrderTypeReq_ = NO_SOT;
    addedDp2SortOrderPartReq_ = NULL;
  }
}

// -----------------------------------------------------------------------
//  RequirementGenerator::removeExtraSortKeys method
//  This method removes key columns that are not covered in the
//  characteristic inputs when atleast the first key column is covered.
//  This method will be called only while creating a context for right
//  child of a NestedJoin. E.g say LC is ordered on
//  'a.b.c.d' and RC is ordered on 'a.b'. Say we are doing a equi join on
//  a and b columnns, then we should consider fully ordered NJ plan. But we
//  don't since columns c and d are not covered,  we return FALSE in the method
//  checkCompatibilityWithGroupAttributes().
// -----------------------------------------------------------------------

void RequirementGenerator::removeExtraSortKeys()
{
  ValueIdSet newExternalInputs=ga_->getCharacteristicInputs();
  if (stillFeasible_ AND addedSortKey_)
  {
    if (addedSortKey_->entries() > 0)
    {
      ValueId firstVid =(*addedSortKey_)[0];
      if (newExternalInputs.contains(firstVid))
      {
        addedSortKey_->removeUnCoveredExprs(ga_->getCharacteristicInputs());
      }
    }
  }
}

void RequirementGenerator::removeArrangement()
{
  addedArrangement_ = NULL;
  removeStartArrangement_ = TRUE;

  // If there is no added sort key requirement, we can also remove any
  // sort order type requirements.
  if (addedSortKey_ == NULL)
  {
    addedSortOrderTypeReq_ = NO_SOT;
    addedDp2SortOrderPartReq_ = NULL;
  }
}

void RequirementGenerator::removeSortOrderTypeReq()
{
  addedSortOrderTypeReq_ = NO_SOT;
  addedDp2SortOrderPartReq_ = NULL;
}

void RequirementGenerator::removeAllPartitioningRequirements()
{
  addedPartKey_.clear();
  partKeyHasBeenAdded_ = FALSE;
  addedNumberOfPartitions_ = ANY_NUMBER_OF_PARTITIONS;
  addedPartitioningRequirement_ = NULL;
  removeStartPartitioningRequirements_ = TRUE;
  generatedPartitioningReqIsValid_ = FALSE;
  requireHash2Only_ = FALSE;
}

void RequirementGenerator::replaceLocationRequirement(PlanExecutionEnum loc)
{
  // note that it is not allowed to remove the location requirement
  // completely (we just decided this because it happens that in the
  // current design there always is a location requirement)
  locationHasBeenAdded_ = TRUE;
  addedLocation_        = loc;
  removeStartLocation_  = TRUE;
}

void RequirementGenerator::makeNumOfPartsFeasible(Lng32 &proposedNumOfParts,
                                  float *proposedNumOfPartsAllowedDeviation)
{
  // rather than just producing a non-feasible requirement by adding some
  // incompatible requirement for the number of partitions, alter the
  // requirement such that it does not conflict with already existing
  // requirements

  // see what partitioning requirement we would generate so far
  producePartitioningRequirements();

  // if the produced part req specifies a fixed number of partitions
  // then return those, otherwise leave proposedNumOfParts unchanged
  if (stillFeasible_ AND
      (generatedPartitioningRequirement_  != NULL) AND
      (generatedPartitioningRequirement_->getCountOfPartitions() !=
        ANY_NUMBER_OF_PARTITIONS))
  {
    proposedNumOfParts = generatedPartitioningRequirement_->
                                       getCountOfPartitions();
    if (generatedPartitioningRequirement_->isRequirementApproximatelyN() AND
        (proposedNumOfPartsAllowedDeviation != NULL))
      *proposedNumOfPartsAllowedDeviation =
         generatedPartitioningRequirement_->
         castToRequireApproximatelyNPartitions()->
         getAllowedDeviation();
  }

}

NABoolean RequirementGenerator::checkFeasibility()
{
  if (NOT stillFeasible_)
    return FALSE;

  // make sure we have seen all requirements
  processStartRequirements();
  if (NOT stillFeasible_)
    return FALSE;

  // orders and arrangements are always checked immediately, except
  // for the condition that they must be covered by the group attributes

  // check partitioning requirements by building them
  producePartitioningRequirements();
  if (NOT stillFeasible_)
    return FALSE;

  // adjust location because part.req. cannot be satisfied in MASTER
  if ( (addedLocation_ == EXECUTE_IN_MASTER_OR_ESP) AND
       ((addedNumberOfPartitions_*(1.0-addedNumOfPartsAllowedDeviation_)) > 1.0)
     )
  {
    addedLocation_ = EXECUTE_IN_ESP;
  }

  // finally
  checkCompatibilityWithGroupAttributes();
  return stillFeasible_;
}

ReqdPhysicalProperty *RequirementGenerator::produceRequirement()
{
  // ---------------------------------------------------------------------
  // NOTE: a NULL return value does NOT mean no requirements, it means
  // that there is not any feasible requirement!
  // ---------------------------------------------------------------------
  if (NOT checkFeasibility())
    {
      // NOTE: don't move the checkFeasibility() call into the assert,
      // it must be executed even when asserts get disabled!!!
      CMPASSERT(0);
      return NULL; // should never get here
    }

  // some sanity checks
  // - we currently always require some execution location
  // - we need at least one CPU to execute
  // - we always specify some (default) cost weight and performance goal
  // - logical partitioning is allowed in DP2 only
  CMPASSERT(locationHasBeenAdded_ AND
	    availableCPUs_ > 0 AND
	    pipelinesPerCPU_ > 0 AND
	    costWeight_ != NULL AND
	    perfGoal_ != NULL AND
	    (addedLocation_ == EXECUTE_IN_DP2 OR logicalPartReq_ == NULL) );

  // If we're in Dp2, there must be no sort order type requirement
  CMPASSERT((addedLocation_ != EXECUTE_IN_DP2) OR
            (addedSortOrderTypeReq_ == NO_SOT));

  generatedRequirement_ = new(CmpCommon::statementHeap())
    ReqdPhysicalProperty(
	   addedArrangement_,
	   addedSortKey_,
           addedSortOrderTypeReq_,
           addedDp2SortOrderPartReq_,
           logicalOrderOrArrangement_,
	   generatedPartitioningRequirement_,
           logicalPartReq_,
	   addedLocation_,
	   availableCPUs_,
	   pipelinesPerCPU_,
	   costWeight_,
	   perfGoal_,
	   NULL);

  generatedRequirement_->setPushDownRequirement(pushDownRequirement_);
  generatedRequirement_->setNoEspExchangeRequirement(requireNoESPExchange_);
  generatedRequirement_->setOcbEnabledCostingRequirement(requireOcbEnabledCosting_);

  return generatedRequirement_;
}

void RequirementGenerator::processStartRequirements()
{
  if (startRequirements_ == NULL OR
      startRequirementsHaveBeenProcessed_ OR
      NOT stillFeasible_)
    return;

  // set this flag first, to prevent anyone else from recursively
  // calling this method and to allow early returns
  startRequirementsHaveBeenProcessed_ = TRUE;

  // add the sort key and arrangement from the start requirements
  if (NOT removeStartSortKey_ AND
      startRequirements_->getSortKey() AND
      (startRequirements_->getSortKey()->entries() > 0))
  {
    addSortKey(*(startRequirements_->getSortKey()),
               startRequirements_->getSortOrderTypeReq(),
               startRequirements_->getDp2SortOrderPartReq());
  }
  if (NOT stillFeasible_)
    return;

  if (NOT removeStartArrangement_ AND
      startRequirements_->getArrangedCols() AND
      (startRequirements_->getArrangedCols()->entries() > 0))
  {
    addArrangement(*(startRequirements_->getArrangedCols()),
                   startRequirements_->getSortOrderTypeReq(),
                   startRequirements_->getDp2SortOrderPartReq());
  }
  if (NOT stillFeasible_)
    return;

  if (NOT removeStartPartitioningRequirements_ AND
      startRequirements_->requiresPartitioning())
  {
    addPartRequirement(startRequirements_->getPartitioningRequirement());
    if (NOT stillFeasible_)
      return;
  }

  addLocationRequirement(startRequirements_->getPlanExecutionLocation());
  if (NOT stillFeasible_)
    return;

  // note that many other fields from the start requirements have already
  // been set in the constructor
} // processStartRequirements()

void RequirementGenerator::producePartitioningRequirements()
{
  // make sure we have all the pieces we need
  processStartRequirements();

  // protect against unnecessary calls
  if (generatedPartitioningReqIsValid_)
    return;

  // Do a consistency check for conflicting requirements, if a fully
  // specified partitioning function is required. Note that we never
  // store a "fuzzy" requirement in data member addedPartitioningRequirement_!
  if (addedPartitioningRequirement_)
  {
    // if both a fully specified requirement (addedPartitioningRequirement_)
    // and a partitioning key and/or number of partitions is required,
    // make sure that both requirements match
    if (partKeyHasBeenAdded_ AND
        NOT addedPartKey_.contains(
               addedPartitioningRequirement_->getPartitioningKey()))
    {
      // The additional required partitioning key is not simply a
      // superset of the fully specified partitioning key. This
      // means that if we would require the fully specified function
      // then its partitioning key would consist of values that
      // are not part of addedPartKey_, therefore not satisfying the
      // requirement that addedPartKey_ be a partitioning key.
      stillFeasible_ = FALSE;
    }

    if ((addedNumberOfPartitions_ != ANY_NUMBER_OF_PARTITIONS) AND
        ((addedPartitioningRequirement_->getCountOfPartitions() >
          addedNumberOfPartitions_) OR
         (addedPartitioningRequirement_->getCountOfPartitions() <
          (addedNumberOfPartitions_ -
           (addedNumberOfPartitions_ * addedNumOfPartsAllowedDeviation_)))
        )
       )
    {
      // the additional requirement for addedNumberOfPartitions_
      // conflicts with the requirement for the hard-coded
      // partitioning function
      stillFeasible_ = FALSE;
    }

    // Check if requireHash2Only_ is set AND partitioning requirement in
    // addedPartitioningRequirement_ is not hash2.
    if (requireHash2Only_ AND !addedPartitioningRequirement_->
                                castToFullySpecifiedPartitioningRequirement()->
                                 getPartitioningFunction()->
                                  isAHash2PartitioningFunction())
    {
      stillFeasible_ = FALSE;
    }

    // if the requirement is still feasible then the partitioning
    // requirement in addedPartitioningRequirement_ is what we want
    // to pass on as partitioning requirement
    if (stillFeasible_)
      generatedPartitioningRequirement_ = addedPartitioningRequirement_;
  }
  else if (partKeyHasBeenAdded_ OR addedNumberOfPartitions_ !=
                                     ANY_NUMBER_OF_PARTITIONS)
  {
    // if we come here, only a partitioning key and/or a number
    // of partitions is required

    if ((partKeyHasBeenAdded_ AND addedPartKey_.isEmpty()) OR
        (addedNumberOfPartitions_ == EXACTLY_ONE_PARTITION))
    {
      // if the partitioning key is empty or if there must be one
      // partition, the only way to satisfy this is with a single
      // partition, so produce a requirement for a single partition

      // if there was a required number of partitions, then 1 partition
      // must satisfy this requirement
      if ((addedNumberOfPartitions_ != ANY_NUMBER_OF_PARTITIONS) AND
          ((addedNumberOfPartitions_ -
            (addedNumberOfPartitions_ * addedNumOfPartsAllowedDeviation_)) >
           EXACTLY_ONE_PARTITION))
        stillFeasible_ = FALSE;
      else
        generatedPartitioningRequirement_ =
          new(CmpCommon::statementHeap())
            RequireExactlyOnePartition();
    }
    else
    {
      // produce a fuzzy partitioning requirement from the
      // given partitioning key and number of partitions
      if (NOT addedPartKey_.isEmpty())
        generatedPartitioningRequirement_ =
          new(CmpCommon::statementHeap())
            RequireApproximatelyNPartitions(addedPartKey_,
                                   addedNumOfPartsAllowedDeviation_,
                                   addedNumberOfPartitions_,
                                   requireHash2Only_);
      else
        generatedPartitioningRequirement_ =
          new(CmpCommon::statementHeap())
            RequireApproximatelyNPartitions(
                                   addedNumOfPartsAllowedDeviation_,
                                   addedNumberOfPartitions_,
                                   requireHash2Only_);
    }

  }
  else
  {
    // otherwise a NULL partitioning requirement is generated
    generatedPartitioningRequirement_ = NULL;
  }

  if ( skewPropertyAdded_ == TRUE ) {
    if ( generatedPartitioningRequirement_ )
       makeSkewReqFeasible(generatedPartitioningRequirement_);
    else
       stillFeasible_ = FALSE;
  }

  generatedPartitioningReqIsValid_ = TRUE;
} // producePartitioningRequirements

void RequirementGenerator::checkCompatibilityWithGroupAttributes()
{
  // some dummy variables for cover test
  ValueIdSet newInputs;
  ValueIdSet referencedInputs;
  ValueIdSet coveredSubExpr;
  ValueIdSet uncoveredExpr;

  // sort key must be covered by the group attributes
  if (stillFeasible_ AND addedSortKey_)
  {
    // remove all required sort columns that are covered by the
    // characteristic inputs, they are constant for this group
    addedSortKey_->removeCoveredExprs(ga_->getCharacteristicInputs());
    if (addedSortKey_->entries() == 0)
    {
      addedSortKey_ = NULL;
      // If there is no added arrangement requirement, we can also remove any
      // sort order type requirements.
      if (addedArrangement_ == NULL)
      {
        addedSortOrderTypeReq_ = NO_SOT;
        addedDp2SortOrderPartReq_ = NULL;
      }
    }
    else
    {
      ValueIdSet sortKey(*addedSortKey_);
      stillFeasible_ = sortKey.isCovered(
        newInputs,
        *ga_,
        referencedInputs,
        coveredSubExpr,
        uncoveredExpr);
      coveredSubExpr.clear();
    }
  }

  // arrangement must be covered by the group attributes
  if (stillFeasible_ AND addedArrangement_ AND
      NOT addedArrangement_->isEmpty())
  {
    // remove all required arrangement columns that are covered by the
    // characteristic inputs, they are constant for this group
    addedArrangement_->removeCoveredExprs(ga_->getCharacteristicInputs());
    if (addedArrangement_->entries() == 0)
    {
      addedArrangement_ = NULL;
      // If there is no added sort order requirement, we can also remove any
      // sort order type requirements.
      if (addedSortKey_ == NULL)
      {
        addedSortOrderTypeReq_ = NO_SOT;
        addedDp2SortOrderPartReq_ = NULL;
      }
    }
    else
    {
      stillFeasible_ = addedArrangement_->isCovered(
        newInputs,
        *ga_,
        referencedInputs,
        coveredSubExpr,
        uncoveredExpr);
      coveredSubExpr.clear();
    }
  }

  // partitioning key must be covered by the group attributes
  if (stillFeasible_ AND generatedPartitioningRequirement_)
    {
      stillFeasible_ = generatedPartitioningRequirement_->
          getPartialPartitioningKey().isCovered(
            newInputs,
	    *ga_,
	    referencedInputs,
	    coveredSubExpr,
	    uncoveredExpr);
      coveredSubExpr.clear();

      // if the partitioning requirement is fuzzy then we can
      // just remove the uncovered expressions from the partitioning
      // key requirement instead of giving up
      // ||opt do that
    }

  // If we are to pass a dp2 sort order partitioning requirement,
  // then it's partitioning key must be covered by the g.a.
  if (stillFeasible_ AND (addedDp2SortOrderPartReq_ != NULL))
  {
    NABoolean isCovered =
      addedDp2SortOrderPartReq_->getPartitioningKey().isCovered(
        newInputs,
        *ga_,
        referencedInputs,
        coveredSubExpr,
        uncoveredExpr);

    coveredSubExpr.clear();

    // If it is not covered, we can try to convert to an ESP_NO_SORT
    // requirement and then get rid of the offending part. requirement
    if (NOT isCovered)
    {
      // see if we can convert to an ESP_NO_SORT requirement
      if (addedSortOrderTypeReq_ == DP2_OR_ESP_NO_SORT_SOT)
      {
        addedSortOrderTypeReq_ = ESP_NO_SORT_SOT;
        addedDp2SortOrderPartReq_ = NULL;
      }
      else
      {
        stillFeasible_ = FALSE;
      }
    } // end if the dp2 sort order part req key is not covered
  } // end if still feasible and a dp2 sort order part req to check

} // checkCompatibilityWithGroupAttributes()

THREAD_P LookupTable<SortOrderTypeEnum>*
  RequirementGenerator::sortOrderTypeReqCompTab_ = NULL;

// This method initializes a lookup table for comparing
// a parent's required sort order type against the child's
// required sort order type. Used to determine a required sort
// order type that is compatible with both the parent and
// child requirements. See the spec
// "Generating Join Plans with Required Orders", section 7.0,
// for more information.
void RequirementGenerator::initSortOrderTypeReqCompTab()
{
  if (sortOrderTypeReqCompTab_ != NULL)
    return;

  sortOrderTypeReqCompTab_ =
    new (GetCliGlobals()->exCollHeap())
      LookupTable<SortOrderTypeEnum>(6,6,GetCliGlobals()->exCollHeap());

  sortOrderTypeReqCompTab_->setValue(0,0,NO_SOT);
  sortOrderTypeReqCompTab_->setValue(0,1,ESP_NO_SORT_SOT);
  sortOrderTypeReqCompTab_->setValue(0,2,ESP_VIA_SORT_SOT);
  sortOrderTypeReqCompTab_->setValue(0,3,DP2_SOT);
  sortOrderTypeReqCompTab_->setValue(0,4,ESP_SOT);
  sortOrderTypeReqCompTab_->setValue(0,5,DP2_OR_ESP_NO_SORT_SOT);

  sortOrderTypeReqCompTab_->setValue(1,0,ESP_NO_SORT_SOT);
  sortOrderTypeReqCompTab_->setValue(1,1,ESP_NO_SORT_SOT);
  sortOrderTypeReqCompTab_->setValue(1,2,INCOMPATIBLE_SOT);
  sortOrderTypeReqCompTab_->setValue(1,3,INCOMPATIBLE_SOT);
  sortOrderTypeReqCompTab_->setValue(1,4,ESP_NO_SORT_SOT);
  sortOrderTypeReqCompTab_->setValue(1,5,ESP_NO_SORT_SOT);

  sortOrderTypeReqCompTab_->setValue(2,0,ESP_VIA_SORT_SOT);
  sortOrderTypeReqCompTab_->setValue(2,1,INCOMPATIBLE_SOT);
  sortOrderTypeReqCompTab_->setValue(2,2,ESP_VIA_SORT_SOT);
  sortOrderTypeReqCompTab_->setValue(2,3,INCOMPATIBLE_SOT);
  sortOrderTypeReqCompTab_->setValue(2,4,ESP_VIA_SORT_SOT);
  sortOrderTypeReqCompTab_->setValue(2,5,INCOMPATIBLE_SOT);

  sortOrderTypeReqCompTab_->setValue(3,0,DP2_SOT);
  sortOrderTypeReqCompTab_->setValue(3,1,INCOMPATIBLE_SOT);
  sortOrderTypeReqCompTab_->setValue(3,2,INCOMPATIBLE_SOT);
  sortOrderTypeReqCompTab_->setValue(3,3,DP2_SOT);
  sortOrderTypeReqCompTab_->setValue(3,4,INCOMPATIBLE_SOT);
  sortOrderTypeReqCompTab_->setValue(3,5,DP2_SOT);

  sortOrderTypeReqCompTab_->setValue(4,0,ESP_SOT);
  sortOrderTypeReqCompTab_->setValue(4,1,ESP_NO_SORT_SOT);
  sortOrderTypeReqCompTab_->setValue(4,2,ESP_VIA_SORT_SOT);
  sortOrderTypeReqCompTab_->setValue(4,3,INCOMPATIBLE_SOT);
  sortOrderTypeReqCompTab_->setValue(4,4,ESP_SOT);
  sortOrderTypeReqCompTab_->setValue(4,5,ESP_NO_SORT_SOT);

  sortOrderTypeReqCompTab_->setValue(5,0,DP2_OR_ESP_NO_SORT_SOT);
  sortOrderTypeReqCompTab_->setValue(5,1,ESP_NO_SORT_SOT);
  sortOrderTypeReqCompTab_->setValue(5,2,INCOMPATIBLE_SOT);
  sortOrderTypeReqCompTab_->setValue(5,3,DP2_SOT);
  sortOrderTypeReqCompTab_->setValue(5,4,ESP_NO_SORT_SOT);
  sortOrderTypeReqCompTab_->setValue(5,5,DP2_OR_ESP_NO_SORT_SOT);

} // initSortOrderTypeReqCompTab()

SortOrderTypeEnum
RequirementGenerator::determineCompatibleSortOrderTypeReq(
                        SortOrderTypeEnum otherSortOrderTypeReq) const
{
  if (sortOrderTypeReqCompTab_ == NULL)
    initSortOrderTypeReqCompTab();

  return sortOrderTypeReqCompTab_->getValue((Int32)otherSortOrderTypeReq,
                                            (Int32)addedSortOrderTypeReq_);
}

void RequirementGenerator::addSortOrderTypeReq(
                           SortOrderTypeEnum sortOrderTypeReq,
                           PartitioningRequirement* dp2SortOrderPartReq)
{
  // If the user did not explicitly specify a sort order type value,
  // then they want the default. Specify a value of NO_SOT if in Dp2
  // and ESP otherwise. The default value for sortOrderTypReq is
  // set to be INCOMPATIBLE_SOT simply to signal that the user did
  // not specify a value and we must specify a real value.
  if (sortOrderTypeReq == INCOMPATIBLE_SOT)
  {
    if (startRequirements_->executeInDP2())
      sortOrderTypeReq = NO_SOT;
    else
      sortOrderTypeReq = ESP_SOT;
  }

  if ((sortOrderTypeReq == DP2_OR_ESP_NO_SORT_SOT) OR
      (sortOrderTypeReq == DP2_SOT))
    CMPASSERT(dp2SortOrderPartReq != NULL);

  // See if what we want to add is compatible with any already added
  // requirement. Returns a type that is compatible for both .
  SortOrderTypeEnum result =
    determineCompatibleSortOrderTypeReq(sortOrderTypeReq);

  if (result == INCOMPATIBLE_SOT)
  {
    stillFeasible_ = FALSE;
    return;
  }

  // if both parent and child specified a Dp2 partitioning
  // requirement, they must be the SAME
  if ((dp2SortOrderPartReq != NULL) AND
      (addedDp2SortOrderPartReq_ != NULL))
  {
    if (addedDp2SortOrderPartReq_->
          comparePartReqToReq(dp2SortOrderPartReq) != SAME)
    {
      // See if we can convert to an ESP_NO_SORT requirement
      if (result == DP2_OR_ESP_NO_SORT_SOT)
      {
        result = ESP_NO_SORT_SOT;
        addedDp2SortOrderPartReq_ = NULL;
      }
      else
      {
        stillFeasible_ = FALSE;
        return;
      }
    }
  }
  else if (dp2SortOrderPartReq != NULL)
  {
    // only the child specified a Dp2 partitioning requirement
    addedDp2SortOrderPartReq_ = dp2SortOrderPartReq;
  }

  // Now, store the resultant sortOrderType
  addedSortOrderTypeReq_ = result;

  // if the resultant sortOrderType now no longer requires a
  // dp2SortOrderPartReq, then set it to NULL.
  if ((addedSortOrderTypeReq_ == ESP_NO_SORT_SOT) OR
      (addedSortOrderTypeReq_ == ESP_VIA_SORT_SOT) OR
      (addedSortOrderTypeReq_ == ESP_SOT))
    addedDp2SortOrderPartReq_ = NULL;

} // addSortOrderTypeReq()

void RequirementGenerator::addNoEspExchangeRequirement()
{
  requireNoESPExchange_ = TRUE;
} // addNoEspExchangeRequirement()

void RequirementGenerator::addOcbCostingRequirement()
{
  requireOcbEnabledCosting_ = TRUE;
} // addOcbCostingRequirement()
